/*
 * DynamicBundleRouter.cpp
 *
 *  Created on: 28.09.2009
 *      Author: morgenro
 */

#include "core/DynamicBundleRouter.h"
#include "net/ConnectionManager.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrcommon/thread/MutexLock.h"

namespace dtn
{
	namespace core
	{
		DynamicBundleRouter::DynamicBundleRouter(list<StaticRoute> routes, BundleStorage &storage)
		 : StaticBundleRouter(routes, storage), _running(true)
		{
		}

		DynamicBundleRouter::~DynamicBundleRouter()
		{
			_wait.enter();
			_running = false;
			_wait.signal(true);
			_wait.leave();
			join();
		}

		void DynamicBundleRouter::signalTimeTick(size_t timestamp)
		{

		}

		void DynamicBundleRouter::signalAvailable(const Node &n)
		{
			ibrcommon::MutexLock l(_wait);
			dtn::data::EID eid(n.getURI());
			_available.push(eid);
			_wait.signal(true);
		}

		void DynamicBundleRouter::signalUnavailable(const Node &n)
		{
			// TODO: remove the node from the available queue
		}

		void DynamicBundleRouter::signalBundleStored(const Bundle &b)
		{
			// get the destination node
			dtn::data::EID dest = b._destination.getNodeEID();

			// get the queue for this destination
			std::queue<dtn::data::BundleID> &q = _stored_bundles[dest];

			// remember the bundle id for later delivery
			q.push( BundleID(b) );
		}

		void DynamicBundleRouter::route(const dtn::data::Bundle &b)
		{
			try {
				StaticBundleRouter::route(b);
			} catch (dtn::exceptions::NoRouteFoundException ex) {
				// No route available, put the bundle into the storage for later delivery
				_storage.store(b);

				// signal the stored bundle
				signalBundleStored(b);
			} catch (dtn::net::ConnectionNotAvailableException ex) {
				// Connection not available, put it into the storage for later delivery
				_storage.store(b);

				// signal the stored bundle
				signalBundleStored(b);
			}
		}

		void DynamicBundleRouter::run()
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
									dtn::data::Bundle b = _storage.get(id);
									StaticBundleRouter::route(b);
									_storage.remove(id);
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
	}
}
