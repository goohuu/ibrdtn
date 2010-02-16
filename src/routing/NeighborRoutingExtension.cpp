/*
 * NeighborRoutingExtension.cpp
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#include "routing/NeighborRoutingExtension.h"
#include "net/BundleReceivedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "core/NodeEvent.h"
#include "core/Node.h"
#include "net/ConnectionManager.h"
#include "ibrcommon/thread/MutexLock.h"

#include <functional>
#include <list>
#include <algorithm>

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
								} catch (dtn::exceptions::NoRouteFoundException ex) {
									bundlequeue.push(id);
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

			if (received != NULL)
			{
				// try to route this bundle
				route(received->getBundleID());
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
			else if (completed != NULL)
			{
				dtn::data::EID eid = completed->getPeer();

				if ( _stored_bundles.find(eid) != _stored_bundles.end() )
				{
					// TODO: if a bundle is delivered remove it from _stored_bundles
					// std::queue<dtn::data::BundleID> &q = _stored_bundles[completed->getPeer()];
				}
			}
		}

		void NeighborRoutingExtension::route(const dtn::data::BundleID &id)
		{
			// get real bundle
			dtn::data::Bundle b = getRouter()->getBundle(id);

			try {
				if 	( isNeighbor( b._destination ) )
				{
					getRouter()->transferTo( b._destination, b );
					return;
				}

				// get the destination node
				dtn::data::EID dest = b._destination.getNodeEID();

				// get the queue for this destination
				std::queue<dtn::data::BundleID> &q = _stored_bundles[dest];

				// remember the bundle id for later delivery
				q.push( BundleID(b) );

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
	}
}
