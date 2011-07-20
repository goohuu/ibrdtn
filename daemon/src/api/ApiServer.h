/*
 * ApiServer.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#ifndef APISERVER_H_
#define APISERVER_H_

#include "Component.h"
#include "api/Registration.h"
#include "api/ClientHandler.h"
#include "core/EventReceiver.h"
#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/net/tcpserver.h>
#include <ibrcommon/thread/Mutex.h>

#include <set>
#include <list>

namespace dtn
{
	namespace api
	{
		class ApiServer : public dtn::daemon::IndependentComponent, public dtn::core::EventReceiver, public ApiServerInterface
		{
		public:
			ApiServer(const ibrcommon::File &socket);
			ApiServer(const ibrcommon::vinterface &net, int port = 4550);
			virtual ~ApiServer();

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

			void freeRegistration(Registration &reg);

			void raiseEvent(const dtn::core::Event *evt);

			void processIncomingBundle(const dtn::data::EID &source, dtn::data::Bundle &bundle);

		protected:
			bool __cancellation();

			virtual void connectionUp(ClientHandler *conn);
			virtual void connectionDown(ClientHandler *conn);

			void componentUp();
			void componentRun();
			void componentDown();

		private:
			ibrcommon::tcpserver _srv;
			std::list<Registration> _registrations;
			std::list<ClientHandler*> _connections;
			ibrcommon::Mutex _connection_lock;
		};
	}
}

#endif /* APISERVER_H_ */
