/*
 * ApiServer.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#ifndef APISERVER_H_
#define APISERVER_H_

#include "ibrdtn/default.h"
#include "daemon/ClientHandler.h"
#include "ibrdtn/utils/tcpserver.h"
#include "ibrdtn/utils/NetInterface.h"

#include <list>

namespace dtn
{
	namespace daemon
	{
		class ApiServer : public dtn::utils::JoinableThread, public dtn::utils::tcpserver
		{
		public:
			ApiServer(dtn::net::NetInterface net);
			virtual ~ApiServer();

		protected:
			void run();

		private:
			bool _running;
		};
	}
}

#endif /* APISERVER_H_ */
