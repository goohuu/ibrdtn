/*
 * FileConvergenceLayer.cpp
 *
 *  Created on: 20.09.2011
 *      Author: morgenro
 */

#include "net/FileConvergenceLayer.h"
#include "net/TransferCompletedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include "core/BundleCore.h"
#include "core/NodeEvent.h"
#include "core/TimeEvent.h"
#include "routing/epidemic/EpidemicControlMessage.h"
#include "routing/RequeueBundleEvent.h"
#include <ibrdtn/data/ScopeControlHopLimitBlock.h>
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/data/File.h>
#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>

namespace dtn
{
	namespace net
	{
		FileConvergenceLayer::Task::Task(FileConvergenceLayer::Task::Action a, const dtn::core::Node &n)
		 : action(a), node(n)
		{
		}

		FileConvergenceLayer::Task::~Task()
		{
		}

		FileConvergenceLayer::StoreBundleTask::StoreBundleTask(const dtn::core::Node &n, const ConvergenceLayer::Job &j)
		 : FileConvergenceLayer::Task(TASK_STORE, n), job(j)
		{
		}

		FileConvergenceLayer::StoreBundleTask::~StoreBundleTask()
		{
		}

		FileConvergenceLayer::FileConvergenceLayer()
		{
		}

		FileConvergenceLayer::~FileConvergenceLayer()
		{
		}

		void FileConvergenceLayer::componentUp()
		{
			bindEvent(dtn::core::NodeEvent::className);
			bindEvent(dtn::core::TimeEvent::className);
		}

		void FileConvergenceLayer::componentDown()
		{
			unbindEvent(dtn::core::NodeEvent::className);
			unbindEvent(dtn::core::TimeEvent::className);
		}

		bool FileConvergenceLayer::__cancellation()
		{
			_tasks.abort();
			return true;
		}

		void FileConvergenceLayer::componentRun()
		{
			try {
				while (true)
				{
					Task *t = _tasks.getnpop(true);

					try {
						switch (t->action)
						{
							case Task::TASK_LOAD:
							{
								// load bundles (receive)
								load(t->node);
								break;
							}

							case Task::TASK_STORE:
							{
								try {
									const StoreBundleTask &sbt = dynamic_cast<const StoreBundleTask&>(*t);
									dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

									// get the file path of the node
									ibrcommon::File path = getPath(sbt.node);

									// scan for bundles
									std::list<dtn::data::MetaBundle> bundles = scan(path);

									try {
										// check if bundle is a epidemic routing bundle
										if (sbt.job._bundle.source == (dtn::core::BundleCore::local + "/routing/epidemic"))
										{
											// read the bundle out of the storage
											const dtn::data::Bundle bundle = storage.get(sbt.job._bundle);

											if (bundle._destination == (sbt.node.getEID() + "/routing/epidemic"))
											{
												// add this bundle to the blacklist
												{
													ibrcommon::MutexLock l(_blacklist_mutex);
													if (_blacklist.find(bundle) != _blacklist.end())
													{
														// send transfer aborted event
														dtn::net::TransferAbortedEvent::raise(sbt.node.getEID(), sbt.job._bundle, dtn::net::TransferAbortedEvent::REASON_REFUSED);
														continue;
													}
													_blacklist.add(bundle);
												}

												// create ECM reply
												replyECM(bundle, bundles);

												// raise bundle event
												dtn::net::TransferCompletedEvent::raise(sbt.node.getEID(), bundle);
												dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
												continue;
											}
										}

										// check if bundle is already in the path
										for (std::list<dtn::data::MetaBundle>::const_iterator iter = bundles.begin(); iter != bundles.end(); iter++)
										{
											if ((*iter) == sbt.job._bundle)
											{
												// send transfer aborted event
												dtn::net::TransferAbortedEvent::raise(sbt.node.getEID(), sbt.job._bundle, dtn::net::TransferAbortedEvent::REASON_REFUSED);
												continue;
											}
										}

										ibrcommon::TemporaryFile filename(path, "bundle");

										try {
											// read the bundle out of the storage
											const dtn::data::Bundle bundle = storage.get(sbt.job._bundle);

											std::fstream fs(filename.getPath().c_str(), std::fstream::out);

											IBRCOMMON_LOGGER(info) << "write bundle " << sbt.job._bundle.toString() << " to file " << filename.getPath() << IBRCOMMON_LOGGER_ENDL;

											dtn::data::DefaultSerializer s(fs);

											// serialize the bundle
											s << bundle;

											// raise bundle event
											dtn::net::TransferCompletedEvent::raise(sbt.node.getEID(), bundle);
											dtn::core::BundleEvent::raise(bundle, dtn::core::BUNDLE_FORWARDED);
										} catch (const ibrcommon::Exception&) {
											filename.remove();
											throw;
										}
									} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {
										// send transfer aborted event
										dtn::net::TransferAbortedEvent::raise(sbt.node.getEID(), sbt.job._bundle, dtn::net::TransferAbortedEvent::REASON_BUNDLE_DELETED);
									} catch (const ibrcommon::Exception&) {
										// something went wrong - requeue transfer for later
										dtn::routing::RequeueBundleEvent::raise(sbt.node.getEID(), sbt.job._bundle);
									}

								} catch (const std::bad_cast&) { }
								break;
							}
						}
					} catch (const std::exception &ex) {
						IBRCOMMON_LOGGER(error) << "error while processing file convergencelayer task: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
					}
					delete t;
				}
			} catch (const ibrcommon::QueueUnblockedException &ex) { };
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
						_tasks.push(new Task(Task::TASK_LOAD, n));
					}
				}
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				if (time.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					ibrcommon::MutexLock l(_blacklist_mutex);
					_blacklist.expire(time.getTimestamp());
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
					IBRCOMMON_LOGGER_DEBUG(34) << "bundle in file " << f.getPath() << " invalid or expired" << IBRCOMMON_LOGGER_ENDL;

					// delete the file
					ibrcommon::File(f).remove();
				}
			}

			return ret;
		}

		void FileConvergenceLayer::queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job)
		{
			_tasks.push(new StoreBundleTask(n, job));
		}

		void FileConvergenceLayer::replyECM(const dtn::data::Bundle &bundle, std::list<dtn::data::MetaBundle> &bl)
		{
			// read the ecm
			const dtn::data::PayloadBlock &p = bundle.getBlock<dtn::data::PayloadBlock>();
			ibrcommon::BLOB::Reference ref = p.getBLOB();
			dtn::routing::EpidemicControlMessage ecm;

			// locked within this region
			{
				ibrcommon::BLOB::iostream s = ref.iostream();
				(*s) >> ecm;
			}

			// if this is a request answer with an summary vector
			if (ecm.type == dtn::routing::EpidemicControlMessage::ECM_QUERY_SUMMARY_VECTOR)
			{
				// create a new request for the summary vector of the neighbor
				dtn::routing::EpidemicControlMessage response_ecm;

				// set message type
				response_ecm.type = dtn::routing::EpidemicControlMessage::ECM_RESPONSE;

				// add own summary vector to the message
				dtn::routing::SummaryVector vec;

				// add bundles in the path
				for (std::list<dtn::data::MetaBundle>::const_iterator iter = bl.begin(); iter != bl.end(); iter++)
				{
					vec.add(*iter);
				}

				// add bundles from the blacklist
				{
					ibrcommon::MutexLock l(_blacklist_mutex);
					for (std::set<dtn::data::MetaBundle>::const_iterator iter = _blacklist.begin(); iter != _blacklist.end(); iter++)
					{
						vec.add(*iter);
					}
				}

				response_ecm.setSummaryVector(vec);

				// add own purge vector to the message
				//response_ecm.setPurgeVector(_purge_vector);

				// create a new bundle
				dtn::data::Bundle answer;

				// set the source of the bundle
				answer._source = bundle._destination;

				// set the destination of the bundle
				answer.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);
				answer._destination = bundle._source;

				// limit the lifetime to 60 seconds
				answer._lifetime = 60;

				// set high priority
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT1, false);
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT2, true);

				dtn::data::PayloadBlock &p = answer.push_back<PayloadBlock>();
				ibrcommon::BLOB::Reference ref = p.getBLOB();

				// serialize the request into the payload
				{
					ibrcommon::BLOB::iostream ios = ref.iostream();
					(*ios) << response_ecm;
				}

				// add a schl block
				dtn::data::ScopeControlHopLimitBlock &schl = answer.push_front<dtn::data::ScopeControlHopLimitBlock>();
				schl.setLimit(1);

				// raise default bundle received event
				dtn::net::BundleReceivedEvent::raise(bundle._destination.getNode(), answer);
			}
		}
	} /* namespace net */
} /* namespace dtn */
