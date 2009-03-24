#include "core/BundleRouter.h"
#include "data/BundleFactory.h"
#include "utils/Utils.h"
#include "core/NodeEvent.h"
#include "core/TimeEvent.h"
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
			: Service("BundleRouter"), m_eid(eid)
		{
			// register at event switch
			EventSwitch::registerEventReceiver(NodeEvent::className, this);
			EventSwitch::registerEventReceiver(RouteEvent::className, this);
			EventSwitch::registerEventReceiver( TimeEvent::className, this );
		}

		BundleRouter::~BundleRouter()
		{
			MutexLock l(m_lock);

			EventSwitch::unregisterEventReceiver(NodeEvent::className, this);
			EventSwitch::unregisterEventReceiver(RouteEvent::className, this);
			EventSwitch::unregisterEventReceiver( TimeEvent::className, this );

			// Alle Nodes löschen
			m_neighbours.clear();
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
					m_nexttime.signal();
				}
			}
			else if (nodeevent != NULL)
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
								EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_DELETED, NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE ) );
							} catch (BundleExpiredException ex) {
								EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
								EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_DELETED, LIFETIME_EXPIRED ) );
							};
						};
						break;
					}
				}
			}
		}

		void BundleRouter::discovered(const Node &node)
		{
			MutexLock l(m_lock);

			// search for the dtn node
			list<Node>::iterator iter = m_neighbours.begin();
			Node n(PERMANENT);

			while (iter != m_neighbours.end())
			{
				n = (*iter);
				if ( n.getURI() ==  node.getURI() )
				{
					// save the node
					(*iter) = node;
					return;
				}

				iter++;
			}

			// not in list, add it.
			m_neighbours.push_back( node );

			// announce the new node
			EventSwitch::raiseEvent(new NodeEvent(node, dtn::core::NODE_AVAILABLE));
		}

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
			// search for outdated nodes
			MutexLock l(m_lock);
			list<Node>::iterator iter = m_neighbours.begin();
			Node n(PERMANENT);

			while (iter != m_neighbours.end())
			{
				n = (*iter);

				if ( !n.decrementTimeout(1) )
				{
					// node is outdated -> remove it
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

			l.unlock();

			m_nexttime.wait();
		}
	}
}
