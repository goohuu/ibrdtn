#include "core/BundleRouter.h"
#include "data/BundleFactory.h"
#include "utils/Utils.h"
#include "core/NodeEvent.h"
#include "core/RouteEvent.h"
#include "core/EventSwitch.h"
#include "core/StorageEvent.h"
#include "core/BundleEvent.h"
#include "core/CustodyEvent.h"
#include "core/BundleCore.h"
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
					case ROUTE_PROCESS_BUNDLE:
					{
						const Bundle &b = routeevent->getBundle();
						BundleCore &core = BundleCore::getInstance();

						if (b.getDestination() == "dtn:none")
						{
							// drop the invalid bundle
							return;
						}

						// extract and parse eid
						EID dest(b.getDestination());

						if (isLocal(b))
						{
							core.deliver( b );
							return;
						}

						try {
							// search for the destination in the neighbourhood
							Node node = getNeighbour(dest.getNodeEID());

							// transmit to neighbour node
							core.transmit(node, b);

							// we're done
							return;
						} catch (NoNeighbourFoundException ex) {
							try {
								// not in the neighbourhood, create a schedule
								BundleSchedule schedule = getSchedule( routeevent->getBundle() );

								try {
									// check if we can transmit it directly
									Node node = getNeighbour( schedule.getEID() );

									// transmit to the neighbouring node
									core.transmit(node, b);

									// we're done
									return;
								} catch (NoNeighbourFoundException ex) {
								};

								// store the schedule for later delivery
								EventSwitch::raiseEvent( new StorageEvent( schedule ) );
							} catch (NoScheduleFoundException ex) {
								EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
								EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_DELETED ) );
							} catch (BundleExpiredException ex) {
								EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
								EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_DELETED ) );
							};
						};
						break;
					}
				}
			}
		}

		/*
		 * Gibt an, dass ein bestimmter Knoten nun in Kommunikationsreichweite ist.
		 */
		void BundleRouter::discovered(const Node &node)
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
		const list<Node>& BundleRouter::getNeighbours()
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

			// check the lifetime of the bundle
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
