#include "core/StaticBundleRouter.h"
#include "core/BundleSchedule.h"
#include "data/BundleFactory.h"
#include "data/EID.h"

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

		BundleSchedule StaticBundleRouter::getSchedule(const Bundle &b)
		{
			try {
				// return a schedule if the default router had found one.
				return BundleRouter::getSchedule(b);
			} catch (NoScheduleFoundException ex) {
				// no default route found, search a route with static informations
				list<StaticRoute>::const_iterator iter = m_routes.begin();

				// check all routes
				while (iter != m_routes.end())
				{
					StaticRoute route = (*iter);
					if ( route.match(b) )
					{
						EID target = EID(route.getDestination());

						// is the next node a neighbor?
						if ( BundleRouter::isNeighbour(route.getDestination()) )
						{
							// Yes, make a schedule for now!
							return BundleSchedule(b, BundleFactory::getDTNTime(), target.getNodeEID());
						}
						else
						{
							// No. Create a schedule with MAX_TIME as time, because it isn't predictable when we meet the node.
							return BundleSchedule(b, BundleSchedule::MAX_TIME, target.getNodeEID());
						}
					}
					iter++;
				}

				// No route possible. Create a schedule with MAX_TIME as time, because it isn't predictable when we meet the node.
				return BundleSchedule(b, BundleSchedule::MAX_TIME, EID(b.getDestination()).getNodeEID() );
			}
		}
	}
}
