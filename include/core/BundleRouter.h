#ifndef BUNDLEROUTER_H_
#define BUNDLEROUTER_H_

#include "ibrdtn/default.h"

#include "core/Node.h"
#include "core/EventReceiver.h"
#include "core/BundleStorage.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/EID.h"

#include <list>

using namespace std;
using namespace dtn::data;

namespace dtn
{
	namespace core
	{
		/**
		 * This is a base class which can be extended to implement several routing
		 * protocols.
		 */
		class BundleRouter : public EventReceiver
		{
		public:
			/**
			 * Constructor
			 */
			BundleRouter(BundleStorage &storage);

			/**
			 * Destructor
			 */
			virtual ~BundleRouter();

			/**
			 * method to receive new events from the EventSwitch
			 */
			virtual void raiseEvent(const Event *evt);

		protected:
			/**
			 * This method do the routing decisions. Every time a bundle is received or
			 * going to transmitted this method gets called. All routing implementation
			 * should overload this method.
			 * @param b The bundle to route.
			 */
			virtual void route(const dtn::data::Bundle &b);

			/**
			 * Determinate if a node with a specific EID exists currently in
			 * the neighborhood.
			 * @param eid The eid of the searched node.
			 * @return True, if the node exists, otherwise it returns false.
			 */
			bool isNeighbor(const EID &eid) const;

			/**
			 * Determine if a bundle is deliverable locally.
			 */
			bool isLocal(const Bundle &b) const;

			virtual void signalAvailable(const Node &n) {};
			virtual void signalUnavailable(const Node &n) {};
			virtual void signalTimeTick(size_t timestamp) {};

			/**
			 * The assigned bundle storage.
			 */
			BundleStorage &_storage;

		private:
			list<Node> m_neighbors;

			ibrcommon::Mutex m_lock;
			ibrcommon::WaitForConditional m_nexttime;
			bool _running;
		};
	}
}

#endif /*BUNDLEROUTER_H_*/
