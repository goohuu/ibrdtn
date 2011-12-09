/*
 * NodeHandshakeExtension.cpp
 *
 *  Created on: 08.12.2011
 *      Author: morgenro
 */

#include "routing/NodeHandshakeExtension.h"
#include "routing/NodeHandshakeEvent.h"

#include "core/NodeEvent.h"
#include "net/ConnectionEvent.h"
#include "core/BundleCore.h"
#include "core/BundleEvent.h"

#include <ibrdtn/data/AgeBlock.h>
#include <ibrdtn/data/ScopeControlHopLimitBlock.h>
#include <ibrdtn/utils/Clock.h>

#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace routing
	{
		NodeHandshakeExtension::NodeHandshakeExtension()
		 : _endpoint(*this)
		{
		}

		NodeHandshakeExtension::~NodeHandshakeExtension()
		{
		}

		void NodeHandshakeExtension::requestHandshake(const dtn::data::EID&, NodeHandshake &request) const
		{
			request.addRequest(BloomFilterPurgeVector::identifier);
		}

		void NodeHandshakeExtension::responseHandshake(const dtn::data::EID&, const NodeHandshake &request, NodeHandshake &answer)
		{
			if (request.hasRequest(BloomFilterSummaryVector::identifier))
			{
				// add own summary vector to the message
				const SummaryVector vec = (**this).getSummaryVector();

				// create an item
				BloomFilterSummaryVector *item = new BloomFilterSummaryVector(vec);

				// add it to the handshake
				answer.addItem(item);
			}

			if (request.hasRequest(BloomFilterPurgeVector::identifier))
			{
				// add own purge vector to the message
				const SummaryVector vec = (**this).getPurgedBundles();

				// create an item
				BloomFilterSummaryVector *item = new BloomFilterSummaryVector(vec);

				// add it to the handshake
				answer.addItem(item);
			}
		}

		void NodeHandshakeExtension::processHandshake(const dtn::data::EID &source, NodeHandshake &answer)
		{
			try {
				const BloomFilterSummaryVector bfsv = answer.get<BloomFilterSummaryVector>();

				// get the summary vector (bloomfilter) of this ECM
				const ibrcommon::BloomFilter &filter = bfsv.getVector().getBloomFilter();

				/**
				 * Update the neighbor database with the received filter.
				 * The filter was sent by the owner, so we assign the contained summary vector to
				 * the EID of the sender of this bundle.
				 */
				NeighborDatabase &db = (**this).getNeighborDB();
				ibrcommon::MutexLock l(db);
				NeighborDatabase::NeighborEntry &entry = db.get(source.getNode());
				entry.update(filter, answer.getLifetime());
			} catch (std::exception&) { };

			try {
				const BloomFilterPurgeVector bfpv = answer.get<BloomFilterPurgeVector>();

				// get the purge vector (bloomfilter) of this ECM
				const ibrcommon::BloomFilter &purge = bfpv.getVector().getBloomFilter();

				dtn::core::BundleStorage &storage = (**this).getStorage();

				while (true)
				{
					// delete bundles in the purge vector
					const dtn::data::MetaBundle meta = storage.remove(purge);

					// log the purged bundle
					IBRCOMMON_LOGGER(notice) << "bundle purged: " << meta.toString() << IBRCOMMON_LOGGER_ENDL;

					// gen a report
					dtn::core::BundleEvent::raise(meta, dtn::core::BUNDLE_DELETED, StatusReportBlock::DEPLETED_STORAGE);

					// add this bundle to the own purge vector
					(**this).addPurgedBundle(meta);
				}
			} catch (std::exception&) { };
		}

		void NodeHandshakeExtension::notify(const dtn::core::Event *evt)
		{
			try {
				// on NodeHandshakeEvent = EXPIRED start a new request
				const NodeHandshakeEvent &handshake = dynamic_cast<const NodeHandshakeEvent&>(*evt);

				if (handshake.state == NodeHandshakeEvent::HANDSHAKE_REQUEST)
				{
					_endpoint.query(handshake.peer);
				}
				return;
			} catch (const std::bad_cast&) { };

			// If a new neighbor comes available, send him a request for the summary vector
			// If a neighbor went away we can free the stored summary vector
			try {
				const dtn::core::NodeEvent &nodeevent = dynamic_cast<const dtn::core::NodeEvent&>(*evt);
				const dtn::core::Node &n = nodeevent.getNode();

				if (nodeevent.getAction() == NODE_AVAILABLE)
				{
					// query a new summary vector from this neighbor
					_endpoint.query(n.getEID());
				}
				else if (nodeevent.getAction() == NODE_UNAVAILABLE)
				{
					// remove the item from the blacklist
					_endpoint.removeFromBlacklist(n.getEID());
				}

				return;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::ConnectionEvent &ce = dynamic_cast<const dtn::net::ConnectionEvent&>(*evt);

				if (ce.state == dtn::net::ConnectionEvent::CONNECTION_UP)
				{
					// query a new summary vector from this neighbor
					_endpoint.query(ce.peer);
				}
				return;
			} catch (const std::bad_cast&) { };
		}

		const std::list<BaseRouter::Extension*>& NodeHandshakeExtension::getExtensions()
		{
			return (**this).getExtensions();
		}

		NodeHandshakeExtension::HandshakeEndpoint::HandshakeEndpoint(NodeHandshakeExtension &callback)
		 : _callback(callback)
		{
			AbstractWorker::initialize("/routing", 50, true);
		}

		NodeHandshakeExtension::HandshakeEndpoint::~HandshakeEndpoint()
		{
		}

		void NodeHandshakeExtension::HandshakeEndpoint::callbackBundleReceived(const Bundle &b)
		{
			_callback.processHandshake(b);
		}

		void NodeHandshakeExtension::HandshakeEndpoint::send(const dtn::data::Bundle &b)
		{
			transmit(b);
		}

		void NodeHandshakeExtension::HandshakeEndpoint::removeFromBlacklist(const dtn::data::EID &eid)
		{
			ibrcommon::MutexLock l(_blacklist_lock);
			_blacklist.erase(eid);
		}

		void NodeHandshakeExtension::HandshakeEndpoint::query(const dtn::data::EID &origin)
		{
			{
				ibrcommon::MutexLock l(_blacklist_lock);
				// only query once each 60 seconds
				if (_blacklist[origin] > dtn::utils::Clock::getUnixTimestamp()) return;
				_blacklist[origin] = dtn::utils::Clock::getUnixTimestamp() + 60;
			}

			// create a new request for the summary vector of the neighbor
			NodeHandshake request(NodeHandshake::HANDSHAKE_REQUEST);

			// walk through all extensions to process the contents of the response
			const std::list<BaseRouter::Extension*>& extensions = _callback.getExtensions();

			for (std::list<BaseRouter::Extension*>::const_iterator iter = extensions.begin(); iter != extensions.end(); iter++)
			{
				BaseRouter::Extension &extension = (**iter);
				extension.requestHandshake(origin, request);
			}

			// create a new bundle
			dtn::data::Bundle req;

			// set the source of the bundle
			req._source = dtn::core::BundleCore::local + "/routing";

			// set the destination of the bundle
			req.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);
			req._destination = origin + "/routing";

			// limit the lifetime to 60 seconds
			req._lifetime = 60;

			// set high priority
			req.set(dtn::data::PrimaryBlock::PRIORITY_BIT1, false);
			req.set(dtn::data::PrimaryBlock::PRIORITY_BIT2, true);

			dtn::data::PayloadBlock &p = req.push_back<PayloadBlock>();
			ibrcommon::BLOB::Reference ref = p.getBLOB();

			// serialize the request into the payload
			{
				ibrcommon::BLOB::iostream ios = ref.iostream();
				(*ios) << request;
			}

			// add a schl block
			dtn::data::ScopeControlHopLimitBlock &schl = req.push_front<dtn::data::ScopeControlHopLimitBlock>();
			schl.setLimit(1);

			// add an age block (to prevent expiring due to wrong clocks)
			req.push_front<dtn::data::AgeBlock>();

			// send the bundle
			transmit(req);
		}

		void NodeHandshakeExtension::processHandshake(const dtn::data::Bundle &bundle)
		{
			// read the ecm
			const dtn::data::PayloadBlock &p = bundle.getBlock<dtn::data::PayloadBlock>();
			ibrcommon::BLOB::Reference ref = p.getBLOB();
			NodeHandshake handshake;

			// locked within this region
			{
				ibrcommon::BLOB::iostream s = ref.iostream();
				(*s) >> handshake;
			}

			// if this is a request answer with an summary vector
			if (handshake.getType() == NodeHandshake::HANDSHAKE_REQUEST)
			{
				// create a new request for the summary vector of the neighbor
				NodeHandshake response(NodeHandshake::HANDSHAKE_RESPONSE);

				// walk through all extensions to process the contents of the response
				const std::list<BaseRouter::Extension*>& extensions = (**this).getExtensions();

				for (std::list<BaseRouter::Extension*>::const_iterator iter = extensions.begin(); iter != extensions.end(); iter++)
				{
					BaseRouter::Extension &extension = (**iter);
					extension.responseHandshake(bundle._source, handshake, response);
				}

				// create a new bundle
				dtn::data::Bundle answer;

				// set the source of the bundle
				answer._source = dtn::core::BundleCore::local + "/routing";

				// set the destination of the bundle
				answer.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);
				answer._destination = bundle._source;

				// limit the lifetime to 60 seconds
				answer._lifetime = 60;

				// set high priority
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT1, false);
				answer.set(dtn::data::PrimaryBlock::PRIORITY_BIT2, true);

				dtn::data::PayloadBlock &p = answer.push_back<PayloadBlock>();
				ibrcommon::BLOB::Reference ref = p.getBLOB();

				// serialize the request into the payload
				{
					ibrcommon::BLOB::iostream ios = ref.iostream();
					(*ios) << response;
				}

				// add a schl block
				dtn::data::ScopeControlHopLimitBlock &schl = answer.push_front<dtn::data::ScopeControlHopLimitBlock>();
				schl.setLimit(1);

				// add an age block (to prevent expiring due to wrong clocks)
				answer.push_front<dtn::data::AgeBlock>();

				// transfer the bundle to the neighbor
				_endpoint.send(answer);

				// call handshake completed event
				NodeHandshakeEvent::raiseEvent( NodeHandshakeEvent::HANDSHAKE_REPLIED, bundle._source );
			}
			else if (handshake.getType() == NodeHandshake::HANDSHAKE_RESPONSE)
			{
				// walk through all extensions to process the contents of the response
				const std::list<BaseRouter::Extension*>& extensions = (**this).getExtensions();

				for (std::list<BaseRouter::Extension*>::const_iterator iter = extensions.begin(); iter != extensions.end(); iter++)
				{
					BaseRouter::Extension &extension = (**iter);
					extension.processHandshake(bundle._source, handshake);
				}

				// call handshake completed event
				NodeHandshakeEvent::raiseEvent( NodeHandshakeEvent::HANDSHAKE_COMPLETED, bundle._source );
			}
		}
	} /* namespace routing */
} /* namespace dtn */
