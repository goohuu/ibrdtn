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
#include "ibrcommon/net/NetInterface.h"
#include "net/GenericServer.h"
#include <ibrdtn/data/MetaBundle.h>
#include <ibrcommon/thread/ThreadSafeQueue.h>

#include <queue>
#include <list>

namespace dtn
{
	namespace daemon
	{
		class ApiServer : public dtn::net::GenericServer<ClientHandler>
		{
		public:
			ApiServer(ibrcommon::NetInterface net, int port = 4550);
			virtual ~ApiServer();

		protected:
			ClientHandler* accept();
			void listen();
			void shutdown();

			void connectionUp(ClientHandler *conn);
			void connectionDown(ClientHandler *conn);

		private:
			class Distributor : public ibrcommon::JoinableThread, public dtn::core::EventReceiver
			{
			public:
				Distributor(std::list<ClientHandler*> &connections, ibrcommon::Mutex &lock);
				~Distributor();

				/**
				 * @see ibrcommon::JoinableThread::run()
				 */
				void run();

				/**
				 * Stop the running daemon
				 */
				void shutdown();

				/**
				 * @see dtn::core::EventReceiver::raiseEvent()
				 */
				void raiseEvent(const dtn::core::Event *evt);

			private:
				bool _running;

				ibrcommon::Mutex &_lock;
				std::list<ClientHandler*> &_connections;
				ibrcommon::ThreadSafeQueue<dtn::data::MetaBundle> _received;
			};

			std::list<ClientHandler*> _connections;
			ibrcommon::Mutex _connection_lock;
			ibrcommon::tcpserver _tcpsrv;
			Distributor _dist;
		};
	}
}

#endif /* APISERVER_H_ */
