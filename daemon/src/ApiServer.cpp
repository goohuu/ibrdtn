/*
 * ApiServer.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ApiServer.h"
#include "core/BundleCore.h"

using namespace dtn::data;
using namespace dtn::core;
using namespace dtn::streams;
using namespace std;

namespace dtn
{
	namespace daemon
	{
		ApiServer::ApiServer(ibrcommon::NetInterface net)
		 : tcpserver(net), _running(true)
		{
		}

		ApiServer::~ApiServer()
		{
			// set running to false
			_running = false;

			// close the tcpserver
			tcpserver::close();

			join();
		}

		void ApiServer::run()
		{
			try {
				while (_running)
				{
					// create a new ClientHandler
					ClientHandler *handler = new ClientHandler( accept() );

					// start the ClientHandler (service)
					handler->start();

					// breakpoint to stop this thread
					yield();
				}
			} catch (ibrcommon::tcpserver::SocketException ex) {
				// socket is closed
			}
		}
	}
}
