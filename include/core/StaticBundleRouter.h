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
				StaticBundleRouter(list<StaticRoute> routes, BundleStorage &storage);
				virtual ~StaticBundleRouter();

				virtual void route(const dtn::data::Bundle &b);

			protected:
				virtual void signalAvailable(const Node &n) {};
				virtual void signalUnavailable(const Node &n) {};
				virtual void signalTimeTick(size_t timestamp) {};
				virtual void signalBundleStored(const Bundle &b) {};

			private:
				list<StaticRoute> m_routes;
		};
	}
}

#endif /*STATICBUNDLEROUTER_H_*/
