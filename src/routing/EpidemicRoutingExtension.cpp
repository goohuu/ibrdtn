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

		void EpidemicRoutingExtension::broadcast(const dtn::data::BundleID &id)
		{
			for (std::list<dtn::core::Node>::const_iterator iter = _neighbors.begin(); iter != _neighbors.end(); iter++)
			{
				const dtn::core::Node &node = (*iter);
				try {
					dtn::data::EID dst(node.getURI());

					// only transfer the bundle if it is not in the bloomfilter of the destination
					if ( !_filterlist[dst].contains(id.toString()) )
					{
						getRouter()->transferTo(EID(node.getURI()), id);
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
			// get the bundle out of the storage
			dtn::data::Bundle bundle = getRouter()->getStorage().get(id);

			// get all epidemic extension blocks of this bundle
			const std::list<EpidemicExtensionBlock> blocks = bundle.getBlocks<EpidemicExtensionBlock>();

			if (!blocks.empty())
			{
				const EpidemicExtensionBlock &ext = blocks.front();
				cout << "Epidemic block found with a bloomfilter based bundle list" << endl;

				// archive the bloom filter for this node!
				_filterlist[bundle._source] = ext.getSummaryVector().getBloomFilter();
			}
		}

		void EpidemicRoutingExtension::run()
		{
			ibrcommon::Mutex l(_wait);

			while (_running)
			{
				bool _bundlelist_changed = false;

				// check for expired bundles
				if (_timestamp > 0)
				{
					ibrcommon::MutexLock l(_list_mutex);
					_seenlist.expire(_timestamp);
					_bundles.expire(_timestamp);
					//_forwarded.expire(_timestamp);
					_timestamp = 0;
				}

				// process new bundles
				while (!_out_queue.empty())
				{
					// get the next bundle
					const dtn::routing::MetaBundle &bundle = _out_queue.front();

					// read routing information
					if (bundle.destination == EID("dtn:epidemic-routing"))
					{
						cout << "got epidemic routing bundle" << endl;
						// read epidemic extension block
						readExtensionBlock(bundle);

						// delete it
						getRouter()->getStorage().remove(bundle);
					}
					else if ( wasSeenBefore(bundle) )
					{
						// ignore/delete the bundle if it was seen before
						getRouter()->getStorage().remove(bundle);
					}
					else
					{
						ibrcommon::MutexLock l(_list_mutex);

						// mark the bundle as seen
						_seenlist.add(bundle);

						// forward this bundle to all neighbors
						broadcast(bundle);

						// put this bundle to the bundle list
						_bundles.add(bundle);
						_bundlelist_changed = true;

						// add this bundle to the summary vector
						_bundle_vector.add(bundle);
					}

					// remove the bundle off the queue
					_out_queue.pop();
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
					broadcast(routingbundle);
				}

				// send all bundles in (_bundles) to all new neighbors (_available)
				while (!_available.empty())
				{
					const dtn::data::EID &eid = _available.front();

					// always transfer the summary vector to new neighbors
					getRouter()->transferTo(eid, routingbundle);

//					ibrcommon::MutexLock l(_list_mutex);
//					{
//						for (std::set<dtn::routing::MetaBundle>::const_iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
//						{
//							const MetaBundle &meta = (*iter);
//
//							try {
//								if (!_forwarded.contains(meta, eid))
//								{
//									getRouter()->transferTo(eid, meta);
//								}
//							} catch (dtn::exceptions::NoBundleFoundException ex) {
//								// remove this bundle from all lists
//								_bundles.remove(meta);
//								_forwarded.remove(meta);
//								_bundle_vector.clear();
//								_bundle_vector.add(_bundles);
//							}
//						}
//					}

					_available.pop();
				}

				yield();
				_wait.wait();
			}
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
				_out_queue.push(received->getBundle());
				_wait.signal(true);
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
				// bundle is expired
				// remove this bundle from seen list
				// remove this bundle from forward-to list
				// BundleList will do this!

				// rebuild the summary vector
				_bundle_vector.clear();
				_bundle_vector.add(_bundles);

			}
			else if (completed != NULL)
			{
				dtn::data::EID eid = completed->getPeer();
				dtn::routing::MetaBundle meta = completed->getBundle();

//				// mark this bundle as forwarded to getPeer()
//				{
//					ibrcommon::MutexLock l(_list_mutex);
//					_filterlist[eid].insert(meta.toString());
//				}

				// delete the bundle in the storage if
				if ( EID(eid.getNodeEID()) == EID(meta.destination) )
				{
					// bundle has been delivered to its destination
					// TODO: generate a "delete" message for routing algorithm

					// delete it from out list
					ibrcommon::MutexLock l(_list_mutex);
					_bundles.remove(meta);
					_bundle_vector.clear();
					_bundle_vector.add(_bundles);

					// delete it from our database
					getRouter()->getStorage().remove(meta);
				}
			}
		}

		bool EpidemicRoutingExtension::wasSeenBefore(const dtn::data::BundleID &id) const
		{
			return ( std::find( _seenlist.begin(), _seenlist.end(), id ) != _seenlist.end());
		}

//		EpidemicRoutingExtension::BundleEIDList::ExpiringList::ExpiringList(const MetaBundle b)
//		 : bundle(b), expiretime(b.getTimestamp() + b.lifetime)
//		{ };
//
//		 EpidemicRoutingExtension::BundleEIDList::ExpiringList::~ExpiringList() {};
//
//		bool EpidemicRoutingExtension::BundleEIDList::ExpiringList::operator!=(const ExpiringList& other) const
//		{
//			return (other.bundle != this->bundle);
//		}
//
//		bool EpidemicRoutingExtension::BundleEIDList::ExpiringList::operator==(const ExpiringList& other) const
//		{
//			return (other.bundle == this->bundle);
//		}
//
//		bool EpidemicRoutingExtension::BundleEIDList::ExpiringList::operator<(const ExpiringList& other) const
//		{
//			return (other.expiretime < expiretime);
//		}
//
//		bool EpidemicRoutingExtension::BundleEIDList::ExpiringList::operator>(const ExpiringList& other) const
//		{
//			return (other.expiretime > expiretime);
//		}
//
//		void EpidemicRoutingExtension::BundleEIDList::ExpiringList::add(const dtn::data::EID eid)
//		{
//			_items.insert(eid);
//		}
//
//		void EpidemicRoutingExtension::BundleEIDList::ExpiringList::remove(const dtn::data::EID eid)
//		{
//			_items.erase(eid);
//		}
//
//		bool EpidemicRoutingExtension::BundleEIDList::ExpiringList::contains(const dtn::data::EID eid) const
//		{
//			return (_items.find(eid) != _items.end());
//		}
//
//
//		EpidemicRoutingExtension::BundleEIDList::BundleEIDList()
//		{};
//
//		EpidemicRoutingExtension::BundleEIDList::~BundleEIDList()
//		{};
//
//		void EpidemicRoutingExtension::BundleEIDList::add(const dtn::routing::MetaBundle bundle, const dtn::data::EID eid)
//		{
//			_bundles.insert(bundle);
//		}
//
//		void EpidemicRoutingExtension::BundleEIDList::remove(const dtn::routing::MetaBundle bundle)
//		{
//			_bundles.erase(bundle);
//		}
//
//		bool EpidemicRoutingExtension::BundleEIDList::contains(const dtn::routing::MetaBundle bundle, const dtn::data::EID eid)
//		{
//			std::set<ExpiringList>::const_iterator iter = _bundles.find(bundle);
//
//			if (iter != _bundles.end())
//			{
//				return (*iter).contains(eid);
//			}
//
//			return false;
//		}
//
//		void EpidemicRoutingExtension::BundleEIDList::expire(const size_t timestamp)
//		{
//			for (std::set<ExpiringList>::iterator iter = _bundles.begin(); iter != _bundles.end(); iter++)
//			{
//				if ((*iter).expiretime >= timestamp )
//				{
//					break;
//				}
//
//				// remove this item from list
//				_bundles.erase( iter );
//			}
//		}

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
