#include "core/StaticBundleRouter.h"
#include "core/BundleSchedule.h"
#include "data/BundleFactory.h"

#include <iostream>

namespace dtn
{
	namespace core
	{
		StaticBundleRouter::StaticBundleRouter(list<StaticRoute> routes, string eid)
			: BundleRouter(eid), m_eid(eid), m_routes(routes)
		{
		}

		StaticBundleRouter::~StaticBundleRouter()
		{
		}

		BundleSchedule StaticBundleRouter::getSchedule(Bundle *b)
		{
			try {
				// Gebe einen Schedule zurück, wenn der StandardRouter einen weg gefunden hat.
				return BundleRouter::getSchedule(b);
			} catch (NoScheduleFoundException ex) {
				// Keinen Standardweg gefunden, suche einen eigenen Weg.
				// Durchlaufe alle statischen Routen
				list<StaticRoute>::const_iterator iter = m_routes.begin();

				while (iter != m_routes.end())
				{
					StaticRoute route = (*iter);
					if ( route.match(b) )
					{
						// Ist der nächste Knoten ein Nachbar?
						if ( BundleRouter::isNeighbour(route.getDestination()) )
						{
							// Ja. Erstelle eine Schedule für jetzt
							return BundleSchedule(b, BundleFactory::getDTNTime(), route.getDestination());
						}
						else
						{
							// Nein. Erzeuge einen Schedule mit maximaler Zeit, nicht gesagt werden kann
							// wann man auf den Knoten trifft.
							return BundleSchedule(b, BundleSchedule::MAX_TIME, route.getDestination());
						}
					}
					iter++;
				}

				// Keine Route passt. Lege das Bundle in die Storage mit maximaler Zeit und mit direkter Adressierung.
				return BundleSchedule(b, BundleSchedule::MAX_TIME, b->getDestination() );
			}
		}
	}
}
