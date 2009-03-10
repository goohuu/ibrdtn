#include "core/BundleRouter.h"
#include "data/BundleFactory.h"
#include "utils/Utils.h"
#include "core/NodeEvent.h"
#include "core/RouteEvent.h"
#include "core/EventSwitch.h"
#include "core/StorageEvent.h"
#include "data/EID.h"
#include <iostream>
#include <iomanip>

using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		BundleRouter::BundleRouter(string eid)
			: Service("BundleRouter"), m_lastcheck(0), m_eid(eid)
		{
			// register at event switch
			EventSwitch::registerEventReceiver(NodeEvent::className, this);
			EventSwitch::registerEventReceiver(RouteEvent::className, this);
		}

		BundleRouter::~BundleRouter()
		{
			MutexLock l(m_lock);

			EventSwitch::unregisterEventReceiver(NodeEvent::className, this);
			EventSwitch::unregisterEventReceiver(RouteEvent::className, this);

			// Alle Nodes löschen
			m_neighbours.clear();
		}

		void BundleRouter::raiseEvent(const Event *evt)
		{
			const NodeEvent *nodeevent = dynamic_cast<const NodeEvent*>(evt);
			const RouteEvent *routeevent = dynamic_cast<const RouteEvent*>(evt);

			if (nodeevent != NULL)
			{
				if (nodeevent->getAction() == NODE_INFO_UPDATED)
				{
					discovered(nodeevent->getNode());
				}
			}
			else if (routeevent != NULL)
			{
				switch (routeevent->getAction())
				{
					case ROUTE_FIND_SCHEDULE:
					{
						const Bundle &b = routeevent->getBundle();
						EID dest(b.getDestination());

						if (isLocal(b))
						{
							EventSwitch::raiseEvent( new RouteEvent( b, dtn::core::ROUTE_LOCAL_BUNDLE ) );
						}

						try {
							// search for the destination in the neighbourhood
							Node node = getNeighbour(dest.getNodeEID());
							BundleSchedule sched( b, 0, dest.getNodeEID() );
							EventSwitch::raiseEvent( new RouteEvent( sched, node ) );
						} catch (NoNeighbourFoundException ex) {
							// not in the neighbourhood, create a schedule
							BundleSchedule schedule = getSchedule( routeevent->getBundle() );
							EventSwitch::raiseEvent( new StorageEvent( schedule ) );
						}

						break;
					}
					case ROUTE_LOCAL_BUNDLE:
						break;
				}

			}
		}

		/*
		 * Gibt an, dass ein bestimmter Knoten nun in Kommunikationsreichweite ist.
		 */
		void BundleRouter::discovered(Node node)
		{
			MutexLock l(m_lock);

			// Suche nach dem DTN Knoten
			list<Node>::iterator iter = m_neighbours.begin();
			Node n(PERMANENT);

			while (iter != m_neighbours.end())
			{
				n = (*iter);
				if ( n.getURI() ==  node.getURI() )
				{
					// Speichern
					(*iter) = node;
					return;
				}

				iter++;
			}

			// Nicht in der Liste. Füge hinzu.
			m_neighbours.push_back( node );

			// announce the new node
			EventSwitch::raiseEvent(new NodeEvent(node, dtn::core::NODE_AVAILABLE));
		}

		/*
		 * Gibt alle direkten Nachbarn zurück
		 */
		list<Node> BundleRouter::getNeighbours()
		{
			MutexLock l(m_lock);
			return m_neighbours;
		}

		bool BundleRouter::isNeighbour(string eid)
		{
			MutexLock l(m_lock);

			list<Node>::iterator iter = m_neighbours.begin();

			while (iter != m_neighbours.end())
			{
				if ( eid.find( (*iter).getURI(), 0) == 0 )
				{
					return true;
				}

				iter++;
			}

			return false;
		}

		Node BundleRouter::getNeighbour(string eid)
		{
			MutexLock l(m_lock);
			list<Node>::iterator iter = m_neighbours.begin();

			while (iter != m_neighbours.end())
			{
				if ( eid.find( (*iter).getURI(), 0) == 0 )
				{
					return (*iter);
				}

				iter++;
			}

			throw NoNeighbourFoundException();
		}

		bool BundleRouter::isNeighbour(Node &node)
		{
			MutexLock l(m_lock);
			list<Node>::iterator iter = m_neighbours.begin();

			while (iter != m_neighbours.end())
			{
				if (node.equals(*iter))
				{
					return true;
				}

				iter++;
			}

			return false;
		}

		BundleSchedule BundleRouter::getSchedule(const Bundle &b)
		{
			if ( b.getDestination() == "dtn:none" )
			{
				throw NoScheduleFoundException("No destination set");
			}

			// Überprüfe die Lebenszeit des Bundles
			if ( b.isExpired() )
			{
				throw BundleExpiredException();
			}

			if 	( isNeighbour( b.getDestination() ) )
			{
				return BundleSchedule(b, 0, EID(b.getDestination()).getNodeEID() );
			}

			if 	( isLocal( b ) )
			{
				return BundleSchedule(b, 0, m_eid );
			}

			//return BundleSchedule(b, BundleSchedule::MAX_TIME, b->getDestination() );

			throw NoScheduleFoundException("No route available");
		}

		bool BundleRouter::isLocal(const Bundle &b) const
		{
			string desteid = b.getDestination();

			if (desteid == m_eid) return true;

			if (desteid.find( m_eid, 0 ) == 0) return true;

			return false;
		}

		void BundleRouter::tick()
		{
			// Suche nach abgelaufenen Nodes
			unsigned int current_time = data::BundleFactory::getDTNTime();

			if ( m_lastcheck != current_time )
			{
				MutexLock l(m_lock);
				list<Node>::iterator iter = m_neighbours.begin();
				Node n(PERMANENT);

				while (iter != m_neighbours.end())
				{
					n = (*iter);

					if ( !n.decrementTimeout(1) )
					{
						// Knoten ist abgelaufen -> entfernen
						list<Node>::iterator eraseme = iter;
						iter++;
						m_neighbours.erase( eraseme );

						// announce the node unavailable event
						EventSwitch::raiseEvent(new NodeEvent(n, dtn::core::NODE_UNAVAILABLE));

						continue;
					}

					(*iter) = n;

					iter++;
				}

				m_lastcheck = current_time;
				m_lock.leaveMutex();
			}

			usleep(5000);
		}
	}
}
