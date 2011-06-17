/*
 * ExtendedApiServer.cpp
 *
 *  Created on: 06.04.2011
 *      Author: morgenro
 */

#include "config.h"
#include "Configuration.h"
#include "api/ExtendedApiServer.h"
#include "api/ExtendedApiConnection.h"
#include "routing/QueueBundleEvent.h"
#include "core/NodeEvent.h"
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace api
	{
		ExtendedApiServer::ExtendedApiServer(const ibrcommon::vinterface &net, int port)
		 : _srv()
		{
			_srv.bind(net, port);
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
			bindEvent(dtn::routing::QueueBundleEvent::className);
			bindEvent(dtn::core::NodeEvent::className);
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

					// send welcome banner
					(*conn) << "IBR-DTN " << dtn::daemon::Configuration::getInstance().version() << " API 1.0" << std::endl;

					ibrcommon::MutexLock l(_connection_lock);

					// create a new registration
					Registration reg; _registrations.push_back(reg);
					IBRCOMMON_LOGGER_DEBUG(5) << "new registration " << reg.getHandle() << IBRCOMMON_LOGGER_ENDL;

					// create a new connection object
					ExtendedApiConnection *eac = new ExtendedApiConnection(*this, _registrations.back(), conn);
					_connections.push_back(eac);

					eac->start();

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
			unbindEvent(dtn::routing::QueueBundleEvent::className);
			unbindEvent(dtn::core::NodeEvent::className);

			{
				ibrcommon::MutexLock l(_connection_lock);

				// shutdown all clients
				for (std::list<ExtendedApiConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					(*iter)->stop();
				}
			}

			// wait until all clients are down
			while (_connections.size() > 0) ::sleep(1);
		}

		const std::string ExtendedApiServer::getName() const
		{
			return "ExtendedApiServer";
		}

		void ExtendedApiServer::connectionUp(ExtendedApiConnection*)
		{
			// generate some output
			IBRCOMMON_LOGGER_DEBUG(5) << "extended api connection up" << IBRCOMMON_LOGGER_ENDL;
		}

		void ExtendedApiServer::connectionDown(ExtendedApiConnection *conn)
		{
			// generate some output
			IBRCOMMON_LOGGER_DEBUG(5) << "extended api connection down" << IBRCOMMON_LOGGER_ENDL;

			ibrcommon::MutexLock l(_connection_lock);

			// remove this object out of the list
			for (std::list<ExtendedApiConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				if (conn == (*iter))
				{
					_connections.erase(iter);
					break;
				}
			}
		}

		void ExtendedApiServer::freeRegistration(Registration &reg)
		{
			// remove the registration
			for (std::list<dtn::api::Registration>::iterator iter = _registrations.begin(); iter != _registrations.end(); iter++)
			{
				if (reg == (*iter))
				{
					IBRCOMMON_LOGGER_DEBUG(5) << "release registration " << reg.getHandle() << IBRCOMMON_LOGGER_ENDL;
					_registrations.erase(iter);
					break;
				}
			}
		}

		void ExtendedApiServer::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);

				ibrcommon::MutexLock l(_connection_lock);
				for (std::list<ExtendedApiConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					ExtendedApiConnection &conn = **iter;
					if (conn.getRegistration().hasSubscribed(queued.bundle.destination))
					{
						conn.getRegistration().queue(queued.bundle);
					}
				}
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::NodeEvent &ne = dynamic_cast<const dtn::core::NodeEvent&>(*evt);

				ibrcommon::MutexLock l(_connection_lock);
				for (std::list<ExtendedApiConnection*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					ExtendedApiConnection &conn = **iter;

					if (ne.getAction() == NODE_AVAILABLE)
					{
						conn.eventNodeAvailable(ne.getNode());
					}
					else if (ne.getAction() == NODE_UNAVAILABLE)
					{
						conn.eventNodeUnavailable(ne.getNode());
					}
				}
			} catch (const std::bad_cast&) { };
		}
	}
}
