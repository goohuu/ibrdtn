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
#include "ibrcommon/net/tcpserver.h"
#include "ibrcommon/net/NetInterface.h"

#include <list>

namespace dtn
{
	namespace daemon
	{
		class ApiServer : public ibrcommon::JoinableThread, public ibrcommon::tcpserver
		{
		public:
			ApiServer(ibrcommon::NetInterface net);
			virtual ~ApiServer();

		protected:
			void run();

		private:
			bool _running;
		};
	}
}

#endif /* APISERVER_H_ */
