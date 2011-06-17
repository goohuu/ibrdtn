/*
 * ExtendedApiServer.h
 *
 *  Created on: 06.04.2011
 *      Author: morgenro
 */

#ifndef EXTENDEDAPISERVER_H_
#define EXTENDEDAPISERVER_H_

#include "api/Registration.h"
#include "core/EventReceiver.h"
#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/net/tcpserver.h>
#include <ibrcommon/thread/Mutex.h>
#include <Component.h>
#include <set>
#include <list>

namespace dtn
{
	namespace api
	{
		class ExtendedApiConnection;

		class ExtendedApiServer : public dtn::daemon::IndependentComponent, public dtn::core::EventReceiver
		{
		public:
			ExtendedApiServer(const ibrcommon::vinterface &net, int port);
			virtual ~ExtendedApiServer();

			bool __cancellation();

			void componentUp();
			void componentRun();
			void componentDown();

			const std::string getName() const;

			void connectionUp(ExtendedApiConnection *conn);
			void connectionDown(ExtendedApiConnection *conn);

			void freeRegistration(Registration &reg);

			void raiseEvent(const dtn::core::Event *evt);

		private:
			ibrcommon::tcpserver _srv;
			std::list<Registration> _registrations;
			std::list<ExtendedApiConnection*> _connections;
			ibrcommon::Mutex _connection_lock;
		};
	}
}

#endif /* EXTENDEDAPISERVER_H_ */
