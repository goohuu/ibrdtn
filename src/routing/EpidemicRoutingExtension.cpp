/*
 * EpidemicRoutingExtension.cpp
 *
 *  Created on: 18.02.2010
 *      Author: morgenro
 */

#include "routing/EpidemicRoutingExtension.h"
#include "routing/MetaBundle.h"
#include "net/BundleReceivedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/NodeEvent.h"
#include "core/TimeEvent.h"
#include "core/Node.h"
#include "net/ConnectionManager.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrcommon/SyslogStream.h"
#include "daemon/Configuration.h"
#include "core/BundleCore.h"

#include <functional>
#include <list>
#include <algorithm>

#include <iomanip>
#include <ios>
#include <iostream>

#include <stdlib.h>

namespace dtn
{
	namespace routing
	{
		struct FindNode: public std::binary_function< dtn::core::Node, dtn::core::Node, bool > {
			bool operator () ( const dtn::core::Node &n1, const dtn::core::Node &n2 ) const {
				return n1.equals(n2);
			}
		};

		EpidemicRoutingExtension::EpidemicRoutingExtension()
		 : _timestamp(0)
		{
			// get configuration
			dtn::daemon::Configuration &conf = dtn::daemon::Configuration::getInstance();

			// write something to the syslog
			ibrcommon::slog << "Initializing epidemic routing module for node " << conf.getNodename() << endl;
		}

		EpidemicRoutingExtension::~EpidemicRoutingExtension()
		{
			stopExtension();
			join();
		}

		void EpidemicRoutingExtension::broadcast(const dtn::data::BundleID &id, bool filter)
		{
			for (std::set<dtn::data::EID>::const_iterator iter = _neighbors.begin(); iter != _neighbors.end(); iter++)
			{
				const dtn::data::EID &dst = (*iter);

				try {
					if (filter)
					{
						std::map<dtn::data::EID, ibrcommon::BloomFilter>::iterator iter = _filterlist.find(dst);
						if (iter != _filterlist.end())
						{
							if (!(*iter).second.contains(id))
							{
								getRouter()->transferTo(dst, id);
							}
						}
						else
						{
							getRouter()->transferTo(dst, id);
						}
					}
					else
					{
						getRouter()->transferTo(dst, id);
					}
				} catch (dtn::exceptions::NoBundleFoundException ex) {

				}
			}
		}

		void EpidemicRoutingExtension::update(std::string &name, std::string &data)
		{
			name = "epidemic";
			data = "version=1";
		}

		void EpidemicRoutingExtension::readExtensionBlock(const dtn::data::BundleID &id)
		{
			try {
				// get the bundle out of the storage
				dtn::data::Bundle bundle = getRouter()->getStorage().get(id);

				// get all epidemic extension blocks of this bundle
				const std::list<EpidemicExtensionBlock> blocks = bundle.getBlocks<EpidemicExtensionBlock>();

				if (!blocks.empty())
				{
					const EpidemicExtensionBlock &ext = blocks.front();
					const ibrcommon::BloomFilter &filter = ext.getSummaryVector().getBloomFilter();

					if (_filterlist.find(bundle._source) == _filterlist.end())
					{
						// check for all bundles in the bloomfilter
						for (std::set<dtn::routing::MetaBundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
						{
							std::string bundleid = (*iter).toString();

							if (!filter.contains(bundleid))
							{
								// always transfer the summary vector to new neighbors
								getRouter()->transferTo( bundle._source, (*iter) );
							}
						}
					}

					// archive the bloom filter for this node!
					_filterlist[bundle._source] = filter;
				}
			} catch (dtn::exceptions::NoBundleFoundException ex) {

			}
		}

		void EpidemicRoutingExtension::remove(const dtn::routing::MetaBundle &meta)
		{
			// lock the lists
			ibrcommon::MutexLock l(_list_mutex);

			// delete it from out list
			_bundles.remove(meta);

			_bundle_vector.clear();
			_bundle_vector.add(_bundles);
		}

		void EpidemicRoutingExtension::notify(const dtn::core::Event *evt)
		{
			const dtn::net::BundleReceivedEvent *received = dynamic_cast<const dtn::net::BundleReceivedEvent*>(evt);
			const dtn::core::NodeEvent *nodeevent = dynamic_cast<const dtn::core::NodeEvent*>(evt);
			const dtn::net::TransferCompletedEvent *completed = dynamic_cast<const dtn::net::TransferCompletedEvent*>(evt);
			const dtn::core::BundleExpiredEvent *expired = dynamic_cast<const dtn::core::BundleExpiredEvent*>(evt);
			const dtn::core::TimeEvent *time = dynamic_cast<const dtn::core::TimeEvent*>(evt);

			if (time != NULL)
			{
				if (time->getAction() == dtn::core::TIME_SECOND_TICK)
				{
					// check all lists for expired entries
					ibrcommon::MutexLock l(_wait);
					_timestamp = time->getTimestamp();
					_wait.signal(true);
				}
			}
			else if (received != NULL)
			{
				// forward a new bundle to all current neighbors
				// and all neighbors in future
				ibrcommon::MutexLock l(_wait);
				_bundle_queue.push(received->getBundle());
				_wait.signal(true);
			}
			else if (nodeevent != NULL)
			{
				dtn::core::Node n = nodeevent->getNode();
				dtn::data::EID eid(n.getURI());

				switch (nodeevent->getAction())
				{
					case NODE_AVAILABLE:
					{
						ibrcommon::MutexLock l(_wait);
						_neighbors.insert(eid);
						_new_neighbors.push(eid);
						_wait.signal(true);
					}
					break;

					case NODE_UNAVAILABLE:
					{
						_neighbors.erase(eid);

						// delete bloomfilter
						_filterlist.erase(eid);
					}
					break;
				}
			}
			else if (expired != NULL)
			{
				// bundle is expired
				// remove this bundle from seen list
				// remove this bundle from forward-to list
				// BundleList will do this!

				// lock the lists
				ibrcommon::MutexLock l(_list_mutex);

				// rebuild the summary vector
				_bundle_vector.clear();
				_bundle_vector.add(_bundles);

			}
			else if (completed != NULL)
			{
				dtn::data::EID eid = completed->getPeer();
				dtn::routing::MetaBundle meta = completed->getBundle();

				// delete the bundle in the storage if
				if ( EID(eid.getNodeEID()) == EID(meta.destination) )
				{
					// bundle has been delivered to its destination
					// TODO: generate a "delete" message for routing algorithm


					// remove the bundle in all lists
					remove(meta);

					// delete it from our database
					getRouter()->getStorage().remove(meta);
				}
			}
		}

		void EpidemicRoutingExtension::run()
		{
			ibrcommon::Mutex l(_wait);

			while (_running)
			{
				std::queue<dtn::routing::MetaBundle> routingdata;
				bool _bundlelist_changed = false;

				// check for expired bundles
				if (_timestamp > 0)
				{
					ibrcommon::MutexLock l(_list_mutex);
					_seenlist.expire(_timestamp);
					_bundles.expire(_timestamp);
					_timestamp = 0;
				}

				// process new bundles
				while (!_bundle_queue.empty())
				{
					// get the next bundle
					const dtn::routing::MetaBundle &bundle = _bundle_queue.front();

					// read routing information
					if (bundle.destination == EID("dtn:epidemic-routing"))
					{
						routingdata.push(bundle);
					}
					else if ( wasSeenBefore(bundle) )
					{
						// ignore/delete the bundle if it was seen before
					}
					else
					{
						ibrcommon::MutexLock l(_list_mutex);

						// mark the bundle as seen
						_seenlist.add(bundle);

						// forward this bundle to all neighbors
						broadcast(bundle, true);

						// put this bundle to the bundle list
						_bundles.add(bundle);
						_bundlelist_changed = true;

						// add this bundle to the summary vector
						_bundle_vector.add(bundle);
					}

					// remove the bundle off the queue
					_bundle_queue.pop();
				}

				/**
				 * create a routing bundle
				 * which contains a summary vector
				 */
				dtn::data::Bundle routingbundle;
				routingbundle._lifetime = 10;
				routingbundle._source = dtn::core::BundleCore::local;
				routingbundle._destination = EID("dtn:epidemic-routing");

				// create a new epidemic block
				EpidemicExtensionBlock *eblock = new EpidemicExtensionBlock(_bundle_vector);
				routingbundle.addBlock(eblock);

				// publish new bundle list to all neighbors
				if ( _bundlelist_changed )
				{
					// forward this bundle to all neighbors
					broadcast(routingbundle, false);

					// delete the neighbor queue
					while (!_new_neighbors.empty()) _new_neighbors.pop();
				}
				else
				{
					// send the routingbundle to new neighbors only
					while (!_new_neighbors.empty())
					{
						// get the first neighbor
						dtn::data::EID &dst = _new_neighbors.front();

						// transfer the bundle to the neighbor
						getRouter()->transferTo(dst, routingbundle);

						// remove the neighbor in the queue
						_new_neighbors.pop();
					}
				}

				// process all new routing data of other nodes
				while (!routingdata.empty())
				{
					// get the next bundle
					const dtn::routing::MetaBundle &bundle = routingdata.front();

					// read epidemic extension block
					readExtensionBlock(bundle);

					// delete it
					getRouter()->getStorage().remove(bundle);

					// remove the element in the queue
					routingdata.pop();
				}

				yield();
				_wait.wait();
			}
		}

		bool EpidemicRoutingExtension::wasSeenBefore(const dtn::data::BundleID &id) const
		{
			return ( std::find( _seenlist.begin(), _seenlist.end(), id ) != _seenlist.end());
		}

		EpidemicRoutingExtension::EpidemicExtensionBlock::EpidemicExtensionBlock(const SummaryVector &vector)
		 : dtn::data::Block(EpidemicExtensionBlock::BLOCK_TYPE), _data("forwarded through epidemic routing"), _vector(vector)
		{
			commit();
		}

		EpidemicRoutingExtension::EpidemicExtensionBlock::EpidemicExtensionBlock(dtn::data::Block *block)
		 : dtn::data::Block(EpidemicExtensionBlock::BLOCK_TYPE, block->getBLOB())
		{
			read();
		}

		EpidemicRoutingExtension::EpidemicExtensionBlock::~EpidemicExtensionBlock()
		{
		}

		const SummaryVector& EpidemicRoutingExtension::EpidemicExtensionBlock::getSummaryVector() const
		{
			return _vector;
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::set(dtn::data::SDNV value)
		{
			_counter = value;
			commit();
		}

		dtn::data::SDNV EpidemicRoutingExtension::EpidemicExtensionBlock::get() const
		{
			return _counter;
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::read()
		{
			// read the attributes out of the BLOB
			ibrcommon::BLOB::Reference ref = dtn::data::Block::getBLOB();
			ibrcommon::MutexLock l(ref);

			(*ref) >> _counter;
			(*ref) >> _data;
			(*ref) >> _vector;
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::commit()
		{
			// read the attributes out of the BLOB
			ibrcommon::BLOB::Reference ref = dtn::data::Block::getBLOB();
			ibrcommon::MutexLock l(ref);
			ref.clear();

			(*ref) << _counter;
			(*ref) << _data;
			(*ref) << _vector;
		}
	}
}
