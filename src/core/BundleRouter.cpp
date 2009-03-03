#include "core/BundleRouter.h"
#include "data/BundleFactory.h"
#include "utils/Utils.h"
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
		}

		BundleRouter::~BundleRouter()
		{
			MutexLock l(m_lock);

			// Alle Nodes löschen
			m_neighbours.clear();
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

		BundleSchedule BundleRouter::getSchedule(Bundle *b)
		{
			if ( b->getDestination() == "dtn:none" )
			{
				throw NoScheduleFoundException("No destination set");
			}

			// Überprüfe die Lebenszeit des Bundles
			if ( b->isExpired() )
			{
				throw BundleExpiredException();
			}

			if 	( isNeighbour( b->getDestination() ) )
			{
				return BundleSchedule(b, 0, b->getDestination() );
			}

			if 	( isLocal( b ) )
			{
				return BundleSchedule(b, 0, m_eid );
			}

			//return BundleSchedule(b, BundleSchedule::MAX_TIME, b->getDestination() );

			throw NoScheduleFoundException("No route available");
		}

		bool BundleRouter::isLocal(Bundle *b)
		{
			string desteid = b->getDestination();

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
