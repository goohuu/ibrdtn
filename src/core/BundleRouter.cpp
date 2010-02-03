#include "core/BundleRouter.h"

#include "ibrdtn/utils/Utils.h"
#include "core/NodeEvent.h"
#include "core/TimeEvent.h"
#include "core/RouteEvent.h"
#include "core/EventSwitch.h"
#include "core/BundleEvent.h"
#include "core/CustodyEvent.h"
#include "core/BundleCore.h"
#include "ibrdtn/data/EID.h"
#include <iostream>
#include <iomanip>
#include <functional>
#include <list>
#include <algorithm>

using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		struct FindNode: public std::binary_function< Node, Node, bool > {
			bool operator () ( const Node &n1, const Node &n2 ) const {
				return n1.equals(n2);
			}
		};

		BundleRouter::BundleRouter(BundleStorage &storage)
		 : _storage(storage)
		{

			// register at event switch
			EventSwitch::registerEventReceiver(NodeEvent::className, this);
			EventSwitch::registerEventReceiver(RouteEvent::className, this);
			EventSwitch::registerEventReceiver( TimeEvent::className, this );
		}

		BundleRouter::~BundleRouter()
		{
			EventSwitch::unregisterEventReceiver(NodeEvent::className, this);
			EventSwitch::unregisterEventReceiver(RouteEvent::className, this);
			EventSwitch::unregisterEventReceiver( TimeEvent::className, this );

			// Alle Nodes löschen
			m_neighbors.clear();
		}

		void BundleRouter::route(const dtn::data::Bundle &b)
		{
			if ( b._destination == EID() )
			{
				throw exceptions::InvalidBundleData("The destination EID is not set.");
			}

			// check the lifetime of the bundle
			if ( b.isExpired() )
			{
				throw exceptions::BundleExpiredException();
			}

			if 	( isLocal( b ) )
			{
				// if a bundle is local, store it for later queries
				_storage.store(b);
				return;
			}

			try {
				if 	( isNeighbor( b._destination ) )
				{
					BundleCore::getInstance().transmit( b._destination, b );
					return;
				}
			} catch (dtn::net::ConnectionNotAvailableException ex) {
				// the connection to the node is not possible
				throw NoRouteFoundException("Connection to the neighboring node is not possible.");
			}

			throw NoRouteFoundException("No route available");
		}

		bool BundleRouter::isLocal(const Bundle &b) const
		{
			if (b._destination.getNodeEID() == BundleCore::local.getNodeEID()) return true;
			return false;
		}

		void BundleRouter::raiseEvent(const Event *evt)
		{
			const NodeEvent *nodeevent = dynamic_cast<const NodeEvent*>(evt);
			const RouteEvent *routeevent = dynamic_cast<const RouteEvent*>(evt);
			const TimeEvent *time = dynamic_cast<const TimeEvent*>(evt);

			if (time != NULL)
			{
				if (time->getAction() == TIME_SECOND_TICK)
				{
					// the time has changed
					signalTimeTick(time->getTimestamp());
				}
			}
			else if (nodeevent != NULL)
			{
				Node n = nodeevent->getNode();

				switch (nodeevent->getAction())
				{
					case NODE_AVAILABLE:
					{
						list<Node>::iterator iter = std::find_if(m_neighbors.begin(), m_neighbors.end(), std::bind2nd( FindNode(), n ) );
						if (iter == m_neighbors.end())
						{
							m_neighbors.push_back(nodeevent->getNode());
						}

						signalAvailable(nodeevent->getNode());
					}
						break;

					case NODE_UNAVAILABLE:
					{
						list<Node>::iterator iter = std::find_if(m_neighbors.begin(), m_neighbors.end(), std::bind2nd( FindNode(), n ) );
						if (iter != m_neighbors.end())
						{
							m_neighbors.erase(iter);
						}

						signalUnavailable(nodeevent->getNode());
					}
						break;
				}
			}
			else if (routeevent != NULL)
			{
				switch (routeevent->getAction())
				{
					case ROUTE_PROCESS_BUNDLE:
					{
						const Bundle &b = routeevent->getBundle();
						try {
							// route the bundle
							route(b);
						} catch (InvalidBundleData ex) {
							// The bundle is invalid. Discard it.
							EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
							EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_DELETED, StatusReportBlock::NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE ) );
						} catch (BundleExpiredException ex) {
							// The is expired. Discard it.
							EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
							EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_DELETED, StatusReportBlock::LIFETIME_EXPIRED ) );
						} catch (NoRouteFoundException ex) {
							// No path found for the bundle. Discard it.
							EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
							EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_DELETED, StatusReportBlock::NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE ) );
						}

						break;
					}
				}
			}
		}

		bool BundleRouter::isNeighbor(const EID &eid) const
		{
			list<Node>::const_iterator iter = m_neighbors.begin();

			while (iter != m_neighbors.end())
			{
				if ( eid.sameHost((*iter).getURI()) )
				{
					return true;
				}

				iter++;
			}

			return false;
		}
	}
}
