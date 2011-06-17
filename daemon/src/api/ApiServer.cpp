/*
 * ApiServer.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "config.h"
#include "api/ApiServer.h"
#include "api/ClientHandler.h"
#include "core/BundleCore.h"
#include "core/EventSwitch.h"
#include "routing/QueueBundleEvent.h"
#include <ibrcommon/Logger.h>
#include <typeinfo>
#include <algorithm>

#ifdef HAVE_SQLITE
#include "core/SQLiteBundleStorage.h"
#endif

namespace dtn
{
	namespace api
	{
		ApiServer::ApiServer(const ibrcommon::File &socket)
		 : _tcpsrv(socket), _next_connection_id(1)
		{
		}

		ApiServer::ApiServer(const ibrcommon::vinterface &net, int port)
		{
			_tcpsrv.bind(net, port);
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
			_tcpsrv.listen(5);

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
			} catch (const std::exception&) {
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
				ibrcommon::MutexLock l(_connections_cond);
				_connections.push_back(obj);
				_connections_cond.signal(true);
			}
			_tasks.push(new QueryBundleTask(obj->id));
		}

		void ApiServer::Distributor::remove(ClientHandler *obj)
		{
			ibrcommon::MutexLock l(_connections_cond);
			_connections.erase( std::remove( _connections.begin(), _connections.end(), obj), _connections.end() );
			_connections_cond.signal(true);
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
#ifdef HAVE_SQLITE
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback, public dtn::core::SQLiteBundleStorage::SQLBundleQuery
#else
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback
#endif
			{
			public:
				BundleFilter(const dtn::data::EID &destination)
				 : _destination(destination)
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 5; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					if (_destination != meta.destination)
					{
						return false;
					}

					return true;
				};

#ifdef HAVE_SQLITE
				const std::string getWhere() const
				{
					return "destination = ?";
				};

				size_t bind(sqlite3_stmt *st, size_t offset) const
				{
					sqlite3_bind_text(st, offset, _destination.getString().c_str(), _destination.getString().size(), SQLITE_TRANSIENT);
					return offset + 1;
				}
#endif

			private:
				const dtn::data::EID &_destination;
			};

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
							dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

							// lock the connection list
							ibrcommon::MutexLock l(_connections_cond);

							for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
							{
								ClientHandler *handler = (*iter);

								if (handler->id == query.id)
								{
									BundleFilter filter(handler->getPeer());
									const std::list<dtn::data::MetaBundle> list = storage.get( filter );

									for (std::list<dtn::data::MetaBundle>::const_iterator iter = list.begin(); iter != list.end(); iter++)
									{
										try {
											const dtn::data::Bundle b = storage.get(*iter);

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
									}

									if (list.size() > 0)
									{
										// generate a task for this receiver
										_tasks.push(new QueryBundleTask(query.id));
									}
								}
							}
						} catch (const std::bad_cast&) {};

						try {
							ProcessBundleTask &pbt = dynamic_cast<ProcessBundleTask&>(*task);

							IBRCOMMON_LOGGER_DEBUG(60) << "ProcessBundleTask: " << pbt.bundle.toString() << IBRCOMMON_LOGGER_ENDL;

							// search for all receiver of this bundle
							ibrcommon::MutexLock l(_connections_cond);
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
							dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

							// search for all receiver of this bundle
							ibrcommon::MutexLock l(_connections_cond);

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
							dtn::core::BundleStorage &storage = dtn::core::BundleCore::getInstance().getStorage();

							storage.remove(r.bundle);

						} catch (const std::bad_cast&) {};
					} catch (const std::exception&) {};

					delete task;

					yield();
				}
			} catch (const ibrcommon::QueueUnblockedException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "ApiServer::Distributor going down" << IBRCOMMON_LOGGER_ENDL;
			} catch (const std::exception&) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected error or shutdown" << IBRCOMMON_LOGGER_ENDL;
			}
		}

		/**
		 * @see dtn::core::EventReceiver::raiseEvent()
		 */
		void ApiServer::Distributor::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);
				_tasks.push(new ApiServer::ProcessBundleTask(queued.bundle));
			} catch (const std::bad_cast&) {

			}
		}

		const std::string ApiServer::getName() const
		{
			return "ApiServer";
		}

		void ApiServer::Distributor::shutdown()
		{
			closeAll();

			// wait until all client connections are down
			ibrcommon::MutexLock l(_connections_cond);
			while (_connections.size() > 0) _connections_cond.wait();
		}

		void ApiServer::Distributor::closeAll()
		{
			// search for an existing connection
			ibrcommon::MutexLock l(_connections_cond);
			for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				ClientHandler &conn = *(*iter);

				// close the connection immediately
				conn.shutdown();
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
