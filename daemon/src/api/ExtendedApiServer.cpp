/*
 * ExtendedApiServer.cpp
 *
 *  Created on: 06.04.2011
 *      Author: morgenro
 */

#include "api/ExtendedApiServer.h"
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace api
	{
		ExtendedApiServer::ExtendedApiServer(const ibrcommon::vinterface &net, int port)
		 : _srv()
		{
			_srv.bind(net, port - 1);
		}

		ExtendedApiServer::~ExtendedApiServer()
		{
		}

		bool ExtendedApiServer::__cancellation()
		{
			_srv.shutdown();
			Thread::interrupt();
			return true;
		}

		void ExtendedApiServer::componentUp()
		{
			_srv.listen(5);
		}

		void ExtendedApiServer::componentRun()
		{
			try {
				while (true)
				{
					// accept the next client
					ibrcommon::tcpstream *conn = _srv.accept();

					// generate some output
					IBRCOMMON_LOGGER_DEBUG(5) << "new connected client at the extended API server" << IBRCOMMON_LOGGER_ENDL;

					// send some info to the client
					(*conn) << "ACCESS DENIED" << std::endl;

					// close the connection
					conn->close();

					// cleanup the object
					delete conn;

					// breakpoint
					ibrcommon::Thread::yield();
				}
			} catch (const std::exception&) {
				// ignore all errors
				return;
			}
		}

		void ExtendedApiServer::componentDown()
		{
		}

		const std::string ExtendedApiServer::getName() const
		{
			return "ExtendedApiServer";
		}
	}
}
