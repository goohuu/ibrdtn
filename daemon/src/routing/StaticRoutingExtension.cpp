/*
 * StaticRoutingExtension.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "routing/StaticRoutingExtension.h"
#include "net/BundleReceivedEvent.h"
#include "net/TransferAbortedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "core/SimpleBundleStorage.h"
#include <typeinfo>

namespace dtn
{
	namespace routing
	{
		StaticRoutingExtension::StaticRoutingExtension(list<StaticRoutingExtension::StaticRoute> routes)
		 : _routes(routes)
		{
			try {
				// scan for bundles in the storage
				dtn::core::SimpleBundleStorage &storage = dynamic_cast<dtn::core::SimpleBundleStorage&>(getRouter()->getStorage());

				std::list<dtn::data::BundleID> list = storage.getList();

				for (std::list<dtn::data::BundleID>::const_iterator iter = list.begin(); iter != list.end(); iter++)
				{
					try {
						dtn::routing::MetaBundle meta( storage.get(*iter) );

						// push the bundle into the queue
						route( meta );
					} catch (dtn::core::BundleStorage::NoBundleFoundException) {
						// error, bundle not found!
					}
				}
			} catch (std::bad_cast ex) {
				// Another bundle storage is used!
			}
		}

		StaticRoutingExtension::~StaticRoutingExtension()
		{

		}

		void StaticRoutingExtension::notify(const dtn::core::Event *evt)
		{
			const dtn::net::BundleReceivedEvent *received = dynamic_cast<const dtn::net::BundleReceivedEvent*>(evt);
			const dtn::net::TransferCompletedEvent *completed = dynamic_cast<const dtn::net::TransferCompletedEvent*>(evt);
			const dtn::net::TransferAbortedEvent *aborted = dynamic_cast<const dtn::net::TransferAbortedEvent*>(evt);

			if (received != NULL)
			{
				// try to route this bundle
				route(received->getBundle());
			}
			else if (completed != NULL)
			{
				// delete bundle in storage
				dtn::core::BundleStorage &storage = getRouter()->getStorage();

				storage.remove(completed->getBundle());
			}
			else if (aborted != NULL)
			{
				// nothing?
			}
		}

		void StaticRoutingExtension::route(const dtn::routing::MetaBundle &meta)
		{
			try {
				// check all routes
				for (std::list<StaticRoute>::const_iterator iter = _routes.begin(); iter != _routes.end(); iter++)
				{
					StaticRoute route = (*iter);
					if ( route.match(meta.destination) )
					{
						dtn::data::EID target = dtn::data::EID(route.getDestination());

						// Yes, make transmit it now!
						getRouter()->transferTo( target, meta);
						return;
					}
				}
			} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
				// bundle may expired, ignore it.
			}
		}

		StaticRoutingExtension::StaticRoute::StaticRoute(string route, string dest)
			: m_route(route), m_dest(dest)
		{
			// prepare for fast matching
			size_t splitter = m_route.find_first_of("*");

			// four types of matching possible
			// 0. no asterisk exists, do exact matching
			// 1. asterisk at the begin
			// 2. asterisk in the middle
			// 3. asterisk at the end
			if ( splitter == string::npos )					m_matchmode = 0;
			else if ( splitter == 0 ) 						m_matchmode = 1;
			else if ( splitter == (m_route.length() -1) ) 	m_matchmode = 3;
			else 											m_matchmode = 2;

			switch (m_matchmode)
			{
				default:

				break;

				case 1:
					m_route1 = m_route.substr(1);
				break;

				case 2:
					m_route1 = m_route.substr(0, splitter);
					m_route2 = m_route.substr(splitter + 1);
				break;

				case 3:
					m_route1 = m_route.substr(0, m_route.length() - 1 );
				break;
			}
		}

		StaticRoutingExtension::StaticRoute::~StaticRoute()
		{
		}

		bool StaticRoutingExtension::StaticRoute::match(const dtn::data::EID &eid)
		{
			string dest = eid.getNodeEID();

			switch (m_matchmode)
			{
				default:
					// exact matching
					if ( dest == m_route ) return true;
				break;

				case 3:
					// asterisk at the end. the begining must be equal.
					if ( dest.find(m_route1, 0) == 0 ) return true;
				break;

				case 2:
					if (
						 ( dest.find(m_route1) == 0 ) &&
						 ( dest.rfind(m_route2) == (dest.length() - m_route2.length()) )
					   )
						return true;
				break;

				case 1:
					// asterisk at begin. the end must be equal.
					if ( dest.rfind(m_route1) == (dest.length() - m_route1.length()) ) return true;
				break;
			}

			return false;
		}

		string StaticRoutingExtension::StaticRoute::getDestination()
		{
			return m_dest;
		}
	}
}
