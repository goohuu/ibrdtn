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

#include <list>

namespace dtn
{
	namespace daemon
	{
		class ApiServer : public IndependentComponent, public ibrcommon::tcpserver
		{
		public:
			ApiServer(ibrcommon::NetInterface net);
			virtual ~ApiServer();

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

		private:
			bool _running;
		};
	}
}

#endif /* APISERVER_H_ */
