#ifndef BUNDLEROUTER_H_
#define BUNDLEROUTER_H_

#include "Node.h"
#include <list>
#include "data/Bundle.h"
#include "utils/Service.h"
#include "core/BundleSchedule.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include "core/EventReceiver.h"
#include "core/Event.h"
#include "utils/Conditional.h"

using namespace std;
using namespace dtn::data;
using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		/**
		 * A bundle router search a route for given bundle and return a BundleSchedule.
		 * Additional manage a list of currently reachable nodes.
		 *
		 * This is a base class which can be extended to implement several routing
		 * protocols.
		 */
		class BundleRouter : public Service, public EventReceiver
		{
		public:
			/* constructor
			 * @param eid Own node eid.
			 */
			BundleRouter(string eid);

			/**
			 * destructor
			 */
			virtual ~BundleRouter();

			/**
			 * Returns a list of currently reachable nodes.
			 * @return The list of currently reachable nodes.
			 */
			virtual const list<Node>& getNeighbours();

			/**
			 * Determine of a specific node is currently reachable.
			 * @param node The node to reach.
			 * @return true, if the given node is reachable.
			 */
			virtual bool isNeighbour(Node &node);
			bool isNeighbour(string eid);

			Node getNeighbour(string eid);

			/**
			 * @sa Service::tick()
			 */
			virtual void tick();

			/**
			 * Search for a route and return a schedule for the given bundle
			 * @param b A bundle to route.
			 * @return A schedule for the given bundle.
			 */
			virtual BundleSchedule getSchedule(const Bundle &b);

			bool isLocal(const Bundle &b) const;

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const Event *evt);

		protected:
			void terminate();

		private:
			/**
			 * Updates the list of reachable nodes. If the node is currently unknown
			 * a NODE_AVAILABLE event will be raised.
			 * @param node The node with new information.
			 */
			virtual void discovered(const Node &node);

			list<Node> m_neighbours;
			string m_eid;
			dtn::utils::Mutex m_lock;

			Conditional m_nexttime;
		};
	}
}

#endif /*BUNDLEROUTER_H_*/
