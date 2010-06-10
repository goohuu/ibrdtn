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
#include "ibrcommon/Logger.h"
#include "Configuration.h"
#include "core/BundleCore.h"
#include "core/SimpleBundleStorage.h"

#include <functional>
#include <list>
#include <algorithm>

#include <iomanip>
#include <ios>
#include <iostream>

#include <stdlib.h>
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

		EpidemicRoutingExtension::EpidemicRoutingExtension()
		 : _timestamp(0)
		{
			// get configuration
			dtn::daemon::Configuration &conf = dtn::daemon::Configuration::getInstance();

			// write something to the syslog
			IBRCOMMON_LOGGER(info) << "Initializing epidemic routing module for node " << conf.getNodename() << IBRCOMMON_LOGGER_ENDL;

			try {
				// scan for bundles in the storage
				dtn::core::SimpleBundleStorage &storage = dynamic_cast<dtn::core::SimpleBundleStorage&>(getRouter()->getStorage());

				std::list<dtn::data::BundleID> list = storage.getList();

				for (std::list<dtn::data::BundleID>::const_iterator iter = list.begin(); iter != list.end(); iter++)
				{
					try {
						dtn::routing::MetaBundle meta( storage.get(*iter) );

						// push the bundle into the queue
						_bundle_queue.push( meta );
					} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
						// error, bundle not found!
					}
				}
			} catch (std::bad_cast ex) {
				// Another bundle storage is used!
			}
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
				} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {

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
				const EpidemicExtensionBlock &ext = bundle.getBlock<EpidemicExtensionBlock>();

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
			} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {

			} catch (dtn::data::Bundle::NoSuchBlockFoundException ex) {

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
				{
					// lock the lists
					ibrcommon::MutexLock l(_list_mutex);
					EpidemicExtensionBlock &eblock = routingbundle.push_back<EpidemicExtensionBlock>();
					eblock.setSummaryVector(_bundle_vector);
				}

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
					try {
						getRouter()->getStorage().remove(bundle);
					} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {

					}

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

		dtn::data::Block* EpidemicRoutingExtension::EpidemicExtensionBlock::Factory::create()
		{
			return new EpidemicRoutingExtension::EpidemicExtensionBlock();
		}

		EpidemicRoutingExtension::EpidemicExtensionBlock::EpidemicExtensionBlock()
		 : dtn::data::Block(EpidemicExtensionBlock::BLOCK_TYPE), _data("forwarded through epidemic routing")
		{
		}

		EpidemicRoutingExtension::EpidemicExtensionBlock::~EpidemicExtensionBlock()
		{
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::setSummaryVector(const SummaryVector &vector)
		{
			_vector = vector;
		}

		const SummaryVector& EpidemicRoutingExtension::EpidemicExtensionBlock::getSummaryVector() const
		{
			return _vector;
		}

		void EpidemicRoutingExtension::EpidemicExtensionBlock::set(dtn::data::SDNV value)
		{
			_counter = value;
		}

		dtn::data::SDNV EpidemicRoutingExtension::EpidemicExtensionBlock::get() const
		{
			return _counter;
		}

		const size_t EpidemicRoutingExtension::EpidemicExtensionBlock::getLength() const
		{
			return _counter.getLength() + _data.getLength() + _vector.getLength();
		}

		std::istream& EpidemicRoutingExtension::EpidemicExtensionBlock::deserialize(std::istream &stream)
		{
			stream >> _counter;
			stream >> _data;
			stream >> _vector;

			// unset block not processed bit
			dtn::data::Block::set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, false);

			return stream;
		}

		std::ostream &EpidemicRoutingExtension::EpidemicExtensionBlock::serialize(std::ostream &stream) const
		{
			stream << _counter;
			stream << _data;
			stream << _vector;

			return stream;
		}
	}
}
