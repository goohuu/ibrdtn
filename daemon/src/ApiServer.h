/*
 * ApiServer.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#ifndef APISERVER_H_
#define APISERVER_H_

#include "Component.h"
#include "ClientHandler.h"
#include "ibrcommon/net/tcpserver.h"
#include <ibrcommon/net/vinterface.h>
#include <ibrdtn/data/MetaBundle.h>
#include <ibrcommon/thread/Queue.h>

#include <queue>
#include <list>

namespace dtn
{
	namespace daemon
	{
		class ApiServer : public dtn::daemon::IndependentComponent, public ApiServerInterface
		{
		public:
			ApiServer(const ibrcommon::File &socket);
			ApiServer(const ibrcommon::vinterface &net, int port = 4550);
			virtual ~ApiServer();

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

		protected:
			bool __cancellation();
			void shutdown();

			virtual void connectionUp(ClientHandler *conn);
			virtual void connectionDown(ClientHandler *conn);

			void componentUp();
			void componentRun();
			void componentDown();

		private:
			class Task
			{
			public:
				virtual ~Task() {};
			};

			class ProcessBundleTask : public Task
			{
			public:
				ProcessBundleTask(const dtn::data::MetaBundle&);
				virtual ~ProcessBundleTask();
				const dtn::data::MetaBundle bundle;
			};

			class TransferBundleTask : public Task
			{
			public:
				TransferBundleTask(const dtn::data::BundleID&, const size_t id);
				virtual ~TransferBundleTask();
				const dtn::data::BundleID bundle;
				const size_t id;
			};

			class QueryBundleTask : public Task
			{
			public:
				QueryBundleTask(const size_t id);
				virtual ~QueryBundleTask();
				const size_t id;
			};

			class RemoveBundleTask : public Task
			{
			public:
				RemoveBundleTask(const dtn::data::BundleID&);
				virtual ~RemoveBundleTask();
				const dtn::data::BundleID bundle;
			};

			class Distributor : public ibrcommon::JoinableThread, public dtn::core::EventReceiver
			{
			public:
				Distributor();
				virtual ~Distributor();

				void add(ClientHandler *obj);
				void remove(ClientHandler *obj);
				void shutdown();

				/**
				 * @see dtn::core::EventReceiver::raiseEvent()
				 */
				void raiseEvent(const dtn::core::Event *evt);

				ibrcommon::Mutex _lock;
				std::list<ClientHandler*> _connections;

			protected:
				/**
				 * @see ibrcommon::JoinableThread::run()
				 */
				void run();

				bool __cancellation();

				ibrcommon::Queue<ApiServer::Task*> _tasks;
			};

			ibrcommon::tcpserver _tcpsrv;
			Distributor _dist;

			size_t _next_connection_id;
		};
	}
}

#endif /* APISERVER_H_ */
