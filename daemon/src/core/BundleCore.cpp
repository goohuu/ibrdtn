#include "config.h"
#include "core/BundleCore.h"
#include "core/GlobalEvent.h"
#include "core/BundleEvent.h"
#include "net/TransferAbortedEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "routing/QueueBundleEvent.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrdtn/data/MetaBundle.h>
#include <ibrdtn/data/Exceptions.h>
#include <ibrdtn/data/EID.h>
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/Logger.h>

#include "limits.h"
#include <iostream>
#include <typeinfo>
#include <stdint.h>

#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#include <ibrdtn/security/PayloadConfidentialBlock.h>
#endif

#ifdef WITH_COMPRESSION
#include <ibrdtn/data/CompressedPayloadBlock.h>
#endif

using namespace dtn::data;
using namespace dtn::utils;
using namespace std;

namespace dtn
{
	namespace core
	{
		dtn::data::EID BundleCore::local;
		size_t BundleCore::blocksizelimit = 0;
		bool BundleCore::forwarding = true;

		BundleCore& BundleCore::getInstance()
		{
			static BundleCore instance;
			return instance;
		}

		BundleCore::BundleCore()
		 : _clock(1), _storage(NULL), _router(NULL)
		{
			/**
			 * evaluate the current local time
			 */
			if (dtn::utils::Clock::getTime() > 0)
			{
				dtn::utils::Clock::quality = 1;
			}
			else
			{
				IBRCOMMON_LOGGER(warning) << "The local clock seems to be wrong. Expiration disabled." << IBRCOMMON_LOGGER_ENDL;
			}

			bindEvent(dtn::routing::QueueBundleEvent::className);
		}

		BundleCore::~BundleCore()
		{
			unbindEvent(dtn::routing::QueueBundleEvent::className);
		}

		void BundleCore::componentUp()
		{
			_connectionmanager.initialize();
			_clock.initialize();

			// start a clock
			_clock.startup();
		}

		void BundleCore::componentDown()
		{
			_connectionmanager.terminate();
			_clock.terminate();
		}

		void BundleCore::setStorage(dtn::core::BundleStorage *storage)
		{
			_storage = storage;
		}

		void BundleCore::setRouter(dtn::routing::BaseRouter *router)
		{
			_router = router;
		}

		dtn::routing::BaseRouter& BundleCore::getRouter() const
		{
			if (_router == NULL)
			{
				throw ibrcommon::Exception("router not set");
			}

			return *_router;
		}

		dtn::core::BundleStorage& BundleCore::getStorage()
		{
			if (_storage == NULL)
			{
				throw ibrcommon::Exception("No bundle storage is set! Use BundleCore::setStorage() to set a storage.");
			}
			return *_storage;
		}

		WallClock& BundleCore::getClock()
		{
			return _clock;
		}

		void BundleCore::transferTo(const dtn::data::EID &destination, const dtn::data::BundleID &bundle)
		{
			try {
				_connectionmanager.queue(destination, bundle);
			} catch (const dtn::net::NeighborNotAvailableException &ex) {
				// signal interruption of the transfer
				dtn::net::TransferAbortedEvent::raise(destination, bundle, dtn::net::TransferAbortedEvent::REASON_CONNECTION_DOWN);
			} catch (const dtn::net::ConnectionNotAvailableException &ex) {
				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(destination, bundle);
			} catch (const ibrcommon::Exception&) {
				dtn::routing::RequeueBundleEvent::raise(destination, bundle);
			}
		}

		void BundleCore::addConvergenceLayer(dtn::net::ConvergenceLayer *cl)
		{
			_connectionmanager.addConvergenceLayer(cl);
		}

		void BundleCore::addConnection(const dtn::core::Node &n)
		{
			_connectionmanager.addConnection(n);
		}

		void BundleCore::removeConnection(const dtn::core::Node &n)
		{
			_connectionmanager.removeConnection(n);
		}

		const std::set<dtn::core::Node> BundleCore::getNeighbors()
		{
			return _connectionmanager.getNeighbors();
		}

		void BundleCore::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);
				const dtn::data::MetaBundle &meta = queued.bundle;

				if (meta.destination == local)
				{
					// if the delivered variable is still false at the end of this block.
					// we send a not delivered report.
					bool delivered = false;

					// process this bundle locally
					dtn::data::Bundle bundle = getStorage().get(meta);

					try {
						// check for a custody signal
						dtn::data::CustodySignalBlock custody = bundle.getBlock<dtn::data::CustodySignalBlock>();
						dtn::data::BundleID id(custody._source, custody._bundle_timestamp.getValue(), custody._bundle_sequence.getValue(), (custody._fragment_length.getValue() > 0), custody._fragment_offset.getValue());
						getStorage().releaseCustody(bundle._source, id);

						IBRCOMMON_LOGGER_DEBUG(5) << "custody released for " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;

						delivered = true;
					} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) {
						// no custody signal available
					}

					if (delivered)
					{
						// gen a report
						dtn::core::BundleEvent::raise(meta, BUNDLE_DELIVERED);
					}
					else
					{
						// gen a report
						dtn::core::BundleEvent::raise(meta, BUNDLE_DELETED, StatusReportBlock::DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE);
					}

					// delete the bundle
					getStorage().remove(meta);
				}
			} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
			} catch (const std::bad_cast&) {}
		}

		void BundleCore::validate(const dtn::data::PrimaryBlock &p) const throw (dtn::data::Validator::RejectedException)
		{
			/*
			 *
			 * TODO: reject a bundle if...
			 * ... it is expired (moved to validate(Bundle) to support AgeBlock for expiration)
			 * ... already in the storage
			 * ... a fragment of an already received bundle in the storage
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */

			// if we do not forward bundles
			if (!BundleCore::forwarding)
			{
				if (!p._destination.sameHost(BundleCore::local))
				{
					// ... we reject all non-local bundles.
					IBRCOMMON_LOGGER(warning) << "non-local bundle rejected: " << p.toString() << IBRCOMMON_LOGGER_ENDL;
					throw dtn::data::Validator::RejectedException("bundle is not local");
				}
			}
		}

		void BundleCore::validate(const dtn::data::Block&, const size_t size) const throw (dtn::data::Validator::RejectedException)
		{
			/*
			 *
			 * reject a block if
			 * ... it exceeds the payload limit
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */

			// check for the size of the block
			if ((BundleCore::blocksizelimit > 0) && (size > BundleCore::blocksizelimit))
			{
				IBRCOMMON_LOGGER(warning) << "bundle rejected: block size of " << size << " is too big" << IBRCOMMON_LOGGER_ENDL;
				throw dtn::data::Validator::RejectedException("block size is too big");
			}
		}

		void BundleCore::validate(const dtn::data::Bundle &b) const throw (dtn::data::Validator::RejectedException)
		{
			/*
			 *
			 * TODO: reject a bundle if
			 * ... the security checks (DTNSEC) failed
			 * ... a checksum mismatch is detected (CRC)
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */

			// check if the bundle is expired
			if (dtn::utils::Clock::isExpired(b))
			{
				IBRCOMMON_LOGGER(warning) << "bundle rejected: bundle has expired (" << b.toString() << ")" << IBRCOMMON_LOGGER_ENDL;
				throw dtn::data::Validator::RejectedException("bundle is expired");
			}

#ifdef WITH_BUNDLE_SECURITY
			// do a fast security check
			try {
				dtn::security::SecurityManager::getInstance().fastverify(b);
			} catch (const dtn::security::SecurityManager::VerificationFailedException &ex) {
				IBRCOMMON_LOGGER(notice) << "bundle rejected: security checks failed: " << b.toString() << IBRCOMMON_LOGGER_ENDL;
				throw dtn::data::Validator::RejectedException("security checks failed");
			}
#endif

			// check for invalid blocks
			const std::list<const dtn::data::Block*> bl = b.getBlocks();
			for (std::list<const dtn::data::Block*>::const_iterator iter = bl.begin(); iter != bl.end(); iter++)
			{
				try {
					const dtn::data::ExtensionBlock &e = dynamic_cast<const dtn::data::ExtensionBlock&>(**iter);

					if (e.get(dtn::data::Block::DELETE_BUNDLE_IF_NOT_PROCESSED))
					{
						// reject the hole bundle
						throw dtn::data::Validator::RejectedException("bundle contains unintelligible blocks");
					}

					if (e.get(dtn::data::Block::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED))
					{
						// transmit status report, because we can not process this block
						dtn::core::BundleEvent::raise(b, BUNDLE_RECEIVED, dtn::data::StatusReportBlock::BLOCK_UNINTELLIGIBLE);
					}
				} catch (const std::bad_cast&) { }
			}
		}

		const std::string BundleCore::getName() const
		{
			return "BundleCore";
		}

		void BundleCore::processBlocks(dtn::data::Bundle &b)
		{
			// walk through the block and process them when needed
			const std::list<const dtn::data::Block*> blist = b.getBlocks();

			for (std::list<const dtn::data::Block*>::const_iterator iter = blist.begin(); iter != blist.end(); iter++)
			{
				const dtn::data::Block &block = (**iter);
				switch (block.getType())
				{
#ifdef WITH_BUNDLE_SECURITY
					case dtn::security::PayloadConfidentialBlock::BLOCK_TYPE:
					{
						// try to decrypt the bundle
						try {
							dtn::security::SecurityManager::getInstance().decrypt(b);
						} catch (const dtn::security::SecurityManager::KeyMissingException&) {
							// decrypt needed, but no key is available
							IBRCOMMON_LOGGER(warning) << "No key available for decrypt bundle." << IBRCOMMON_LOGGER_ENDL;
						} catch (const dtn::security::SecurityManager::DecryptException &ex) {
							// decrypt failed
							IBRCOMMON_LOGGER(warning) << "Decryption of bundle failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
						}
					}
#endif

#ifdef WITH_COMPRESSION
					case dtn::data::CompressedPayloadBlock::BLOCK_TYPE:
					{
						// try to decompress the bundle
						try {
							dtn::data::CompressedPayloadBlock::extract(b);
						} catch (const ibrcommon::Exception&) { };
					}
#endif
				}
			}
		}
	}
}
