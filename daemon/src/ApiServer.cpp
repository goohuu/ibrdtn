/*
 * ApiServer.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ApiServer.h"
#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "routing/QueueBundleEvent.h"
#include "ClientHandler.h"
#include <ibrcommon/Logger.h>
#include <typeinfo>
#include <algorithm>
#include <signal.h>

using namespace dtn::data;
using namespace dtn::core;
using namespace dtn::streams;
using namespace std;

namespace dtn
{
	namespace daemon
	{
		ApiServer::ApiServer(const ibrcommon::File &socket)
		 : _tcpsrv(socket), _dist(), _next_connection_id(1)
		{
		}

		ApiServer::ApiServer(const ibrcommon::vinterface &net, int port)
		 : _tcpsrv(net, port), _dist()
		{
		}

		ApiServer::~ApiServer()
		{
			join();
		}

		bool ApiServer::__cancellation()
		{
			shutdown();
			return true;
		}

		void ApiServer::componentUp()
		{
			try {
				_dist.start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start ApiServer\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void ApiServer::componentRun()
		{
			try {
				while (true)
				{
					ClientHandler *obj = new ClientHandler(*this, _tcpsrv.accept(), _next_connection_id);
					_next_connection_id++;

					// initialize the object
					obj->initialize();

					// breakpoint
					ibrcommon::Thread::yield();
				}
			} catch (std::exception) {
				// ignore all errors
				return;
			}
		}

		void ApiServer::componentDown()
		{
			_dist.shutdown();
			shutdown();
		}

		void ApiServer::shutdown()
		{
			_dist.stop();
			_tcpsrv.shutdown();
			_tcpsrv.close();
			Thread::kill(SIG_UNBLOCK);
		}

		void ApiServer::connectionUp(ClientHandler *obj)
		{
			_dist.add(obj);
			IBRCOMMON_LOGGER_DEBUG(5) << "client connection up (id: " << obj->id << ")" << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG(60) << "current open connections " << _dist._connections.size() << IBRCOMMON_LOGGER_ENDL;
		}

		void ApiServer::connectionDown(ClientHandler *obj)
		{
			_dist.remove(obj);
			IBRCOMMON_LOGGER_DEBUG(5) << "client connection down (id: " << obj->id << ")" << IBRCOMMON_LOGGER_ENDL;
			IBRCOMMON_LOGGER_DEBUG(60) << "current open connections " << _dist._connections.size() << IBRCOMMON_LOGGER_ENDL;
		}

		ApiServer::Distributor::Distributor()
		{
			bindEvent(dtn::routing::QueueBundleEvent::className);
		}

		ApiServer::Distributor::~Distributor()
		{
			unbindEvent(dtn::routing::QueueBundleEvent::className);
			join();
		}

		void ApiServer::Distributor::add(ClientHandler *obj)
		{
			{
				ibrcommon::MutexLock l(_lock);
				_connections.push_back(obj);
			}
			_tasks.push(new QueryBundleTask(obj->id));
		}

		void ApiServer::Distributor::remove(ClientHandler *obj)
		{
			ibrcommon::MutexLock l(_lock);
			_connections.erase( std::remove( _connections.begin(), _connections.end(), obj), _connections.end() );
		}

		bool ApiServer::Distributor::__cancellation()
		{
			// cancel the main thread in here
			_tasks.abort();

			// return true, to signal that no further cancel (the hardway) is needed
			return true;
		}

		/**
		 * @see ibrcommon::JoinableThread::run()
		 */
		void ApiServer::Distributor::run()
		{
			try
			{
				while (true)
				{
					// get the next task
					ApiServer::Task *task = _tasks.getnpop(true);

					try {
						try {
							QueryBundleTask &query = dynamic_cast<QueryBundleTask&>(*task);

							IBRCOMMON_LOGGER_DEBUG(60) << "QueryBundleTask: " << query.id << IBRCOMMON_LOGGER_ENDL;

							// get the global storage
							BundleStorage &storage = BundleCore::getInstance().getStorage();

							// lock the connection list
							ibrcommon::MutexLock l(_lock);

							for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
							{
								ClientHandler *handler = (*iter);

								if (handler->id == query.id)
								{
									const dtn::data::EID &eid = handler->getPeer();
									const dtn::data::BundleID id = storage.getByDestination( eid, true );

									try {
										const dtn::data::Bundle b = storage.get(id);

										// push the bundle to the client
										handler->queue(b);

										// generate a delete bundle task
										_tasks.push(new RemoveBundleTask(b));
									}
									catch (const dtn::core::BundleStorage::BundleLoadException&)
									{
									}
									catch (const dtn::core::BundleStorage::NoBundleFoundException&)
									{
										break;
									}

									// generate a task for this receiver
									_tasks.push(new QueryBundleTask(query.id));
								}
							}
						} catch (const std::bad_cast&) {};

						try {
							ProcessBundleTask &pbt = dynamic_cast<ProcessBundleTask&>(*task);

							IBRCOMMON_LOGGER_DEBUG(60) << "ProcessBundleTask: " << pbt.bundle.toString() << IBRCOMMON_LOGGER_ENDL;

							// search for all receiver of this bundle
							ibrcommon::MutexLock l(_lock);
							bool deleteIt = false;

							for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
							{
								ClientHandler *handler = (*iter);

								if (handler->getPeer() == pbt.bundle.destination)
								{
									// generate a task for each receiver
									_tasks.push(new TransferBundleTask(pbt.bundle, handler->id));

									deleteIt = true;
								}
							}

							// generate a delete bundle task
							if (deleteIt) _tasks.push(new RemoveBundleTask(pbt.bundle));

						} catch (const std::bad_cast&) { };

						try {
							TransferBundleTask &transfer = dynamic_cast<TransferBundleTask&>(*task);

							IBRCOMMON_LOGGER_DEBUG(60) << "TransferBundleTask: " << transfer.bundle.toString() << ", id: " << transfer.id << IBRCOMMON_LOGGER_ENDL;

							// get the global storage
							BundleStorage &storage = BundleCore::getInstance().getStorage();

							// search for all receiver of this bundle
							ibrcommon::MutexLock l(_lock);

							for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
							{
								ClientHandler *handler = (*iter);

								try {
									if (handler->id == transfer.id)
									{
										const dtn::data::Bundle b = storage.get( transfer.bundle );

										// push the bundle to the client
										handler->queue(b);
									}
								} catch (const dtn::core::BundleStorage::NoBundleFoundException&) {};
							}

						} catch (const std::bad_cast&) { };

						try {
							RemoveBundleTask &r = dynamic_cast<RemoveBundleTask&>(*task);

							IBRCOMMON_LOGGER_DEBUG(60) << "RemoveBundleTask: " << r.bundle.toString() << IBRCOMMON_LOGGER_ENDL;

							// get the global storage
							BundleStorage &storage = BundleCore::getInstance().getStorage();

							storage.remove(r.bundle);

						} catch (const std::bad_cast&) {};
					} catch (const std::exception&) {};

					delete task;

					yield();
				}
			} catch (const ibrcommon::QueueUnblockedException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ApiServer::Distributor going down" << IBRCOMMON_LOGGER_ENDL;
			} catch (std::exception) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected error or shutdown" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		/**
		 * @see dtn::core::EventReceiver::raiseEvent()
		 */
		void ApiServer::Distributor::raiseEvent(const Event *evt)
		{
			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);
				_tasks.push(new ApiServer::ProcessBundleTask(queued.bundle));
			} catch (std::bad_cast ex) {

			}
		}

		const std::string ApiServer::getName() const
		{
			return "ApiServer";
		}

		void ApiServer::Distributor::shutdown()
		{
			while (true)
			{
				ClientHandler *client = NULL;
				{
					ibrcommon::MutexLock l(_lock);
					if (_connections.empty()) break;
					client = _connections.front();
					_connections.remove(client);
				}

				client->shutdown();
			}
		}

		ApiServer::ProcessBundleTask::ProcessBundleTask(const dtn::data::MetaBundle &b) : bundle(b) {}
		ApiServer::ProcessBundleTask::~ProcessBundleTask(){}

		ApiServer::TransferBundleTask::TransferBundleTask(const dtn::data::BundleID &b, const size_t i) : bundle(b), id(i) {}
		ApiServer::TransferBundleTask::~TransferBundleTask() {}

		ApiServer::RemoveBundleTask::RemoveBundleTask(const dtn::data::BundleID &b) : bundle(b) {}
		ApiServer::RemoveBundleTask::~RemoveBundleTask() {}

		ApiServer::QueryBundleTask::QueryBundleTask(const size_t i) : id(i) {}
		ApiServer::QueryBundleTask::~QueryBundleTask() {}
	}
}
