/*
 * FileConvergenceLayer.cpp
 *
 *  Created on: 20.09.2011
 *      Author: morgenro
 */

#include "net/FileConvergenceLayer.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "core/BundleEvent.h"
#include "core/BundleCore.h"
#include "core/NodeEvent.h"
#include <ibrdtn/data/ScopeControlHopLimitBlock.h>
#include "net/BundleReceivedEvent.h"
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace net
	{
		FileConvergenceLayer::FileConvergenceLayer()
		{
		}

		FileConvergenceLayer::~FileConvergenceLayer()
		{
		}

		void FileConvergenceLayer::componentUp()
		{
			bindEvent(dtn::core::NodeEvent::className);
		}

		void FileConvergenceLayer::componentDown()
		{
			unbindEvent(dtn::core::NodeEvent::className);
		}

		void FileConvergenceLayer::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::core::NodeEvent &node = dynamic_cast<const dtn::core::NodeEvent&>(*evt);

				if (node.getAction() == dtn::core::NODE_AVAILABLE)
				{
					const dtn::core::Node &n = node.getNode();
					if ( n.has(dtn::core::Node::CONN_FILE) )
					{
						// scan and delete expired bundles
						scan(getPath(n));

						// load bundles (receive)
						load(n);
					}
				}
			} catch (const std::bad_cast&) { };
		}

		const std::string FileConvergenceLayer::getName() const
		{
			return "FileConvergenceLayer";
		}

		dtn::core::Node::Protocol FileConvergenceLayer::getDiscoveryProtocol() const
		{
			return dtn::core::Node::CONN_FILE;
		}

		void FileConvergenceLayer::open(const dtn::core::Node&)
		{
		}

		void FileConvergenceLayer::load(const dtn::core::Node &n)
		{
			std::list<dtn::data::MetaBundle> ret;
			std::list<ibrcommon::File> files;

			// list all files in the folder
			getPath(n).getFiles(files);

			for (std::list<ibrcommon::File>::const_iterator iter = files.begin(); iter != files.end(); iter++)
			{
				const ibrcommon::File &f = (*iter);

				// skip system files
				if (f.isSystem()) continue;

				try {
					// open the file
					std::fstream fs(f.getPath().c_str(), std::fstream::in);

					// get a deserializer
					dtn::data::DefaultDeserializer d(fs, dtn::core::BundleCore::getInstance());

					dtn::data::Bundle bundle;

					// load meta data
					d >> bundle;

					// check the bundle
					if ( ( bundle._destination == EID() ) || ( bundle._source == EID() ) )
					{
						// invalid bundle!
						throw dtn::data::Validator::RejectedException("destination or source EID is null");
					}

					// increment value in the scope control hop limit block
					try {
						dtn::data::ScopeControlHopLimitBlock &schl = bundle.getBlock<dtn::data::ScopeControlHopLimitBlock>();
						schl.increment();
					} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) { }

					// raise default bundle received event
					dtn::net::BundleReceivedEvent::raise(n.getEID(), bundle);
				}
				catch (const dtn::data::Validator::RejectedException &ex)
				{
					// display the rejection
					IBRCOMMON_LOGGER(warning) << "bundle has been rejected: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
				catch (const dtn::InvalidDataException &ex) {
					// display the rejection
					IBRCOMMON_LOGGER(warning) << "invalid bundle-data received: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				}
			}
		}

		ibrcommon::File FileConvergenceLayer::getPath(const dtn::core::Node &n)
		{
			std::list<dtn::core::Node::URI> uris = n.get(dtn::core::Node::CONN_FILE);

			// abort the transfer, if no URI exists
			if (uris.size() == 0) throw ibrcommon::Exception("path not defined");

			// get the URI of the file path
			const std::string &uri = uris.front().value;

			if (uri.substr(0, 7) != "file://") throw ibrcommon::Exception("path invalid");

			return ibrcommon::File(uri.substr(7, uri.length() - 7));
		}

		std::list<dtn::data::MetaBundle> FileConvergenceLayer::scan(const ibrcommon::File &path)
		{
			std::list<dtn::data::MetaBundle> ret;
			std::list<ibrcommon::File> files;

			// list all files in the folder
			path.getFiles(files);

			for (std::list<ibrcommon::File>::const_iterator iter = files.begin(); iter != files.end(); iter++)
			{
				const ibrcommon::File &f = (*iter);

				// skip system files
				if (f.isSystem()) continue;

				try {
					// open the file
					std::fstream fs(f.getPath().c_str(), std::fstream::in);

					// get a deserializer
					dtn::data::DefaultDeserializer d(fs);

					dtn::data::MetaBundle meta;

					// load meta data
					d >> meta;

					if (meta.expiretime < dtn::utils::Clock::getTime())
					{
						dtn::core::BundleEvent::raise(meta, dtn::core::BUNDLE_DELETED, dtn::data::StatusReportBlock::LIFETIME_EXPIRED);
						throw ibrcommon::Exception("bundle is expired");
					}

					// put the meta bundle in the list
					ret.push_back(meta);
				} catch (const std::exception&) {
					IBRCOMMON_LOGGER(error) << "bundle in file " << f.getPath() << " invalid or expired" << IBRCOMMON_LOGGER_ENDL;

					// delete the file
					ibrcommon::File(f).remove();
				}
			}

			return ret;
		}

		void FileConvergenceLayer::queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job)
		{
			dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

			try {
				ibrcommon::File path = getPath(n);

				// scan for bundles
				std::list<dtn::data::MetaBundle> bundles = scan(path);

				for (std::list<dtn::data::MetaBundle>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
				{
					if ((*iter) == job._bundle)
					{
						// send transfer aborted event
						dtn::net::TransferAbortedEvent::raise(n.getEID(), job._bundle, dtn::net::TransferAbortedEvent::REASON_REFUSED);
						return;
					}
				}

				ibrcommon::TemporaryFile filename(path, "bundle");

				try {
					// read the bundle out of the storage
					const dtn::data::Bundle bundle = storage.get(job._bundle);

					IBRCOMMON_LOGGER(info) << "write bundle " << job._bundle.toString() << " to file " << filename.getPath() << IBRCOMMON_LOGGER_ENDL;

					std::fstream fs(filename.getPath().c_str(), std::fstream::out);

					dtn::data::DefaultSerializer s(fs);

					// serialize the bundle
					s << bundle;

					// raise bundle event
					dtn::net::TransferCompletedEvent::raise(job._destination, bundle);
					dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
				} catch (const ibrcommon::Exception&) {
					filename.remove();
					throw;
				}
			} catch (const ibrcommon::Exception&) {
				// send transfer aborted event
				dtn::net::TransferAbortedEvent::raise(n.getEID(), job._bundle, dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN);
			}
		}
	} /* namespace net */
} /* namespace dtn */
