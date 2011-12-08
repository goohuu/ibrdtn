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
#include <ibrcommon/thread/Timer.h>

#include <set>
#include <list>

namespace dtn
{
	namespace api
	{
		class ApiServer : public dtn::daemon::IndependentComponent, public dtn::core::EventReceiver, public ApiServerInterface, public ibrcommon::TimerCallback
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

			/**
			 * retrieve a registration for a given handle from the ApiServers registration list
			 * the registration is automatically set into the attached state
			 * @param handle the handle of the registration to return
			 * @exception Registration::AlreadyAttachedException the registration is already attached to a different client
			 * @exception Registration::NotFoundException no registration was found for the given handle
			 * @return the registration with the given handle
			 */
			Registration& getRegistration(const std::string &handle);

			/**
			 * Removes expired registrations.
			 * @exception ibrcommon::Timer::StopTimerException thrown, if the GarbageCollector should be stopped
			 * @see ibrcommon::TimerCallback::timeout(Timer*)
			 * @return the seconds till the GarbageCollector should be triggered next
			 */
			virtual size_t timeout(ibrcommon::Timer*);

		protected:
			bool __cancellation();

			virtual void connectionUp(ClientHandler *conn);
			virtual void connectionDown(ClientHandler *conn);

			void componentUp();
			void componentRun();
			void componentDown();

		private:

			/**
			 * starts the garbage collector if there are persistent registrations and it is not yet running
			 */
			void startGarbageCollector();
			/**
			 * calculates when the next registration expires
			 * @exception ibrcommon::Timer::StopTimerException if no more registrations expire
			 * @return time in seconds until next expiry
			 */
			size_t nextRegistrationExpiry();

			ibrcommon::tcpserver _srv;
			std::list<Registration> _registrations;
			std::list<ClientHandler*> _connections;
			ibrcommon::Mutex _connection_lock;
			ibrcommon::Mutex _registration_lock;
			ibrcommon::Timer _garbage_collector;
		};
	}
}

#endif /* APISERVER_H_ */
