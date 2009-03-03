#ifndef STATICBUNDLEROUTER_H_
#define STATICBUNDLEROUTER_H_

#include "core/BundleRouter.h"
#include "core/StaticRoute.h"
#include <list>

namespace dtn
{
	namespace core
	{
		class StaticBundleRouter : public BundleRouter
		{
			public:
				StaticBundleRouter(list<StaticRoute> routes, string eid);
				~StaticBundleRouter();
			
				BundleSchedule getSchedule(Bundle *b);
				
			private:
				string m_eid;
				list<StaticRoute> m_routes;
		};
	}
}
	
#endif /*STATICBUNDLEROUTER_H_*/
