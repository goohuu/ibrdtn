#include "core/StaticBundleRouter.h"
#include "core/BundleCore.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/utils/Utils.h"

#include <iostream>

namespace dtn
{
	namespace core
	{
		StaticBundleRouter::StaticBundleRouter(list<StaticRoute> routes)
			: BundleRouter(), m_routes(routes)
		{
		}

		StaticBundleRouter::~StaticBundleRouter()
		{
		}

		void StaticBundleRouter::route(const dtn::data::Bundle &b)
		{
			try{
				BundleRouter::route(b);
			} catch (exceptions::NoRouteFoundException ex) {
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
						if ( BundleRouter::isNeighbor(target) )
						{
							// Yes, make transmit it now!
							BundleCore::getInstance().transmit( target, b );
							return;
						}
						else
						{
							// No. Say that there is no route.
							throw exceptions::NoRouteFoundException("No route available");
						}
					}
					iter++;
				}

				// No route possible.
				throw exceptions::NoRouteFoundException("No route available");
			}
		}
	}
}
