/*
 * NeighborRoutingExtension.h
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef NEIGHBORROUTINGEXTENSION_H_
#define NEIGHBORROUTINGEXTENSION_H_

#include "routing/BaseRouter.h"
#include <ibrdtn/data/MetaBundle.h>
#include "ibrdtn/data/EID.h"
#include "core/Node.h"
#include <ibrcommon/thread/Queue.h>
#include <list>
#include <map>
#include <queue>

namespace dtn
{
	namespace routing
	{
		class NeighborRoutingExtension : public BaseRouter::ThreadedExtension
		{
		public:
			NeighborRoutingExtension();
			virtual ~NeighborRoutingExtension();

			void notify(const dtn::core::Event *evt);

			virtual void stopExtension();

		protected:
			void run();
			bool __cancellation();

		private:
			/**
			 * Determinate if a node with a specific EID exists currently in
			 * the neighborhood.
			 * @param eid The eid of the searched node.
			 * @return True, if the node exists, otherwise it returns false.
			 */
			bool isNeighbor(const dtn::data::EID &eid) const;

			/**
			 * Remove a bundle from local lists.
			 * this is necessary if a bundle is expired or another routing
			 * module has delivered the bundle.
			 * @param id
			 */
			void remove(const dtn::data::BundleID &id);

			void route(const dtn::data::MetaBundle &meta);

			std::list<dtn::core::Node> _neighbors;

			ibrcommon::Mutex _stored_bundles_lock;
			std::map<dtn::data::EID, std::queue<dtn::data::BundleID> > _stored_bundles;
			ibrcommon::Queue<dtn::data::EID> _available;
		};
	}
}

#endif /* NEIGHBORROUTINGEXTENSION_H_ */
