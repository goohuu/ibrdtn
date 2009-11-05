/*
 * DynamicBundleRouter.h
 *
 *  Created on: 28.09.2009
 *      Author: morgenro
 */

#ifndef DYNAMICBUNDLEROUTER_H_
#define DYNAMICBUNDLEROUTER_H_

#include "ibrdtn/default.h"
#include "core/StaticBundleRouter.h"
#include "core/BundleStorage.h"
#include "ibrdtn/utils/Thread.h"
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/Conditional.h"
#include <queue>
#include <map>

namespace dtn
{
	namespace core
	{
		class DynamicBundleRouter : public StaticBundleRouter, public dtn::utils::JoinableThread
		{
		public:
			DynamicBundleRouter(list<StaticRoute> routes, BundleStorage &storage);
			virtual ~DynamicBundleRouter();

			virtual void route(const dtn::data::Bundle &b);

		protected:
			virtual void run();

			virtual void signalAvailable(const Node &n);
			virtual void signalUnavailable(const Node &n);
			virtual void signalTimeTick(size_t timestamp);

		private:
			BundleStorage &_storage;
			std::map<dtn::data::EID, std::queue<dtn::data::BundleID> > _stored_bundles;

			bool _running;

			std::queue<dtn::data::EID> _available;
			dtn::utils::Conditional _wait;
		};
	}
}

#endif /* DYNAMICBUNDLEROUTER_H_ */
