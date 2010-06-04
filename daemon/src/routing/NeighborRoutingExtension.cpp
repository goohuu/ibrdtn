/*
 * NeighborRoutingExtension.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "routing/NeighborRoutingExtension.h"
#include "net/BundleReceivedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/NodeEvent.h"
#include "core/Node.h"
#include "net/ConnectionManager.h"
#include "ibrcommon/thread/MutexLock.h"
#include "core/SimpleBundleStorage.h"

#include <functional>
#include <list>
#include <algorithm>
#include <typeinfo>

namespace dtn
{
	namespace routing
	{
		struct FindNode: public std::binary_function< dtn::core::Node, dtn::core::Node, bool > {
			bool operator () ( const dtn::core::Node &n1, const dtn::core::Node &n2 ) const {
				return n1.equals(n2);
			}
		};

		NeighborRoutingExtension::NeighborRoutingExtension()
		{
			try {
				// scan for bundles in the storage
				dtn::core::SimpleBundleStorage &storage = dynamic_cast<dtn::core::SimpleBundleStorage&>(getRouter()->getStorage());

				std::list<dtn::data::BundleID> list = storage.getList();

				for (std::list<dtn::data::BundleID>::const_iterator iter = list.begin(); iter != list.end(); iter++)
				{
					try {
						dtn::routing::MetaBundle meta( storage.get(*iter) );

						// push the bundle into the queue
						route( meta );
					} catch (dtn::core::BundleStorage::NoBundleFoundException) {
						// error, bundle not found!
					}
				}
			} catch (std::bad_cast ex) {
				// Another bundle storage is used!
			}
		}

		NeighborRoutingExtension::~NeighborRoutingExtension()
		{
			stopExtension();
			join();
		}

		void NeighborRoutingExtension::run()
		{
			ibrcommon::Mutex l(_wait);

			while (_running)
			{
				while (!_available.empty())
				{
					EID eid = _available.front();
					_available.pop();

					if ( _stored_bundles.find(eid) != _stored_bundles.end() )
					{
						std::queue<dtn::data::BundleID> &bundlequeue = _stored_bundles[eid];

						while (!bundlequeue.empty())
						{
							dtn::data::BundleID id = bundlequeue.front();
							bundlequeue.pop();

							if ( isNeighbor(eid) )
							{
								try {
									getRouter()->transferTo(eid, id);
								} catch (BaseRouter::NoRouteFoundException ex) {
									bundlequeue.push(id);
								} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
									// bundle may expired, ignore it.
								}
							}
							else break;
						}
					}
				}

				yield();
				_wait.wait();
			}
		}

		void NeighborRoutingExtension::notify(const dtn::core::Event *evt)
		{
			const dtn::net::BundleReceivedEvent *received = dynamic_cast<const dtn::net::BundleReceivedEvent*>(evt);
			const dtn::core::NodeEvent *nodeevent = dynamic_cast<const dtn::core::NodeEvent*>(evt);
			const dtn::net::TransferCompletedEvent *completed = dynamic_cast<const dtn::net::TransferCompletedEvent*>(evt);
			const dtn::core::BundleExpiredEvent *expired = dynamic_cast<const dtn::core::BundleExpiredEvent*>(evt);

			if (received != NULL)
			{
				// try to route this bundle
				route(received->getBundle());
			}
			else if (nodeevent != NULL)
			{
				dtn::core::Node n = nodeevent->getNode();

				switch (nodeevent->getAction())
				{
					case NODE_AVAILABLE:
					{
						std::list<dtn::core::Node>::iterator iter = std::find_if(_neighbors.begin(), _neighbors.end(), std::bind2nd( FindNode(), n ) );
						if (iter == _neighbors.end())
						{
							_neighbors.push_back(nodeevent->getNode());
						}

						ibrcommon::MutexLock l(_wait);
						dtn::data::EID eid(n.getURI());
						_available.push(eid);
						_wait.signal(true);
					}
					break;

					case NODE_UNAVAILABLE:
					{
						std::list<dtn::core::Node>::iterator iter = std::find_if(_neighbors.begin(), _neighbors.end(), std::bind2nd( FindNode(), n ) );
						if (iter != _neighbors.end())
						{
							_neighbors.erase(iter);
						}
					}
					break;
				}
			}
			else if (expired != NULL)
			{
				remove(expired->_bundle);
			}
			else if (completed != NULL)
			{
				dtn::data::EID eid = completed->getPeer();

				if ( _stored_bundles.find(eid) != _stored_bundles.end() )
				{
					// if a bundle is delivered remove it from _stored_bundles
					remove(completed->getBundle());
				}
			}
		}

		void NeighborRoutingExtension::route(const dtn::routing::MetaBundle &meta)
		{
			try {
				// get the destination node
				dtn::data::EID dest = meta.destination.getNodeEID();

				if 	( isNeighbor( dest ) )
				{
					getRouter()->transferTo( dest, meta );
					return;
				}

				// get the queue for this destination
				std::queue<dtn::data::BundleID> &q = _stored_bundles[dest];

				// remember the bundle id for later delivery
				q.push( meta );

			} catch (dtn::net::ConnectionNotAvailableException ex) {
				// the connection to the node is not possible

			}
		}

		bool NeighborRoutingExtension::isNeighbor(const dtn::data::EID &eid) const
		{
			std::list<dtn::core::Node>::const_iterator iter = _neighbors.begin();

			while (iter != _neighbors.end())
			{
				if ( eid.sameHost((*iter).getURI()) )
				{
					return true;
				}

				iter++;
			}

			return false;
		}

		void NeighborRoutingExtension::remove(const dtn::data::BundleID &id)
		{
			for (std::map<dtn::data::EID, std::queue<dtn::data::BundleID> >::iterator iter = _stored_bundles.begin(); iter != _stored_bundles.end(); iter++ )
			{
				std::queue<dtn::data::BundleID> &q = (*iter).second;
				std::queue<dtn::data::BundleID> new_queue;

				while (!q.empty())
				{
					dtn::data::BundleID bid = q.front();

					if (bid != id)
					{
						new_queue.push(bid);
					}

					q.pop();
				}

				_stored_bundles[(*iter).first] = new_queue;
			}
		}
	}
}
