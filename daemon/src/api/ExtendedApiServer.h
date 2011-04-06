/*
 * ExtendedApiServer.h
 *
 *  Created on: 06.04.2011
 *      Author: morgenro
 */

#ifndef EXTENDEDAPISERVER_H_
#define EXTENDEDAPISERVER_H_

#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/net/tcpserver.h>
#include <Component.h>

namespace dtn
{
	namespace api
	{
		class ExtendedApiServer : public dtn::daemon::IndependentComponent
		{
		public:
			ExtendedApiServer(const ibrcommon::vinterface &net, int port);
			virtual ~ExtendedApiServer();

			bool __cancellation();

			void componentUp();
			void componentRun();
			void componentDown();

			const std::string getName() const;

		private:
			ibrcommon::tcpserver _srv;
		};
	}
}

#endif /* EXTENDEDAPISERVER_H_ */
