#include "core/BundleCore.h"
#include "core/GlobalEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "routing/QueueBundleEvent.h"
#include "core/BundleEvent.h"
#include "core/TimeEvent.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrdtn/data/MetaBundle.h>
#include <ibrdtn/data/Exceptions.h>
#include <ibrdtn/data/EID.h>
#include <ibrdtn/utils/Clock.h>
#include <ibrcommon/Logger.h>

#include "limits.h"
#include <iostream>
#include <typeinfo>
#include <stdint.h>

using namespace dtn::data;
using namespace dtn::utils;
using namespace std;

namespace dtn
{
	namespace core
	{
		dtn::data::EID BundleCore::local;
		size_t BundleCore::blocksizelimit = 0;
		bool BundleCore::forwarding = true;

		BundleCore& BundleCore::getInstance()
		{
			static BundleCore instance;
			return instance;
		}

		BundleCore::BundleCore()
		 : _clock(1), _storage(NULL)
		{
			/**
			 * evaluate the current local time
			 */
			if (dtn::utils::Clock::getTime() > 0)
			{
				dtn::utils::Clock::quality = 1;
			}
			else
			{
				IBRCOMMON_LOGGER(warning) << "The local clock seems to be wrong. Expiration disabled." << IBRCOMMON_LOGGER_ENDL;
			}

			bindEvent(dtn::routing::QueueBundleEvent::className);
			bindEvent(dtn::core::TimeEvent::className);
		}

		BundleCore::~BundleCore()
		{
			unbindEvent(dtn::routing::QueueBundleEvent::className);
			unbindEvent(dtn::core::TimeEvent::className);
		}

		void BundleCore::componentUp()
		{
			_connectionmanager.initialize();
			_clock.initialize();

			// start a clock
			_clock.startup();
		}

		void BundleCore::componentDown()
		{
			_connectionmanager.terminate();
			_clock.terminate();
		}

		void BundleCore::setStorage(dtn::core::BundleStorage *storage)
		{
			_storage = storage;
		}

		dtn::core::BundleStorage& BundleCore::getStorage()
		{
			return *_storage;
		}

		WallClock& BundleCore::getClock()
		{
			return _clock;
		}

		void BundleCore::transferTo(const dtn::data::EID &destination, const dtn::data::BundleID &bundle)
		{
			try {
				_connectionmanager.queue(destination, bundle);
			} catch (dtn::net::NeighborNotAvailableException ex) {
				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(destination, bundle);
			} catch (dtn::net::ConnectionNotAvailableException ex) {
				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(destination, bundle);
			} catch (ibrcommon::Exception) {
				dtn::routing::RequeueBundleEvent::raise(destination, bundle);
			}
		}

		void BundleCore::addConvergenceLayer(dtn::net::ConvergenceLayer *cl)
		{
			_connectionmanager.addConvergenceLayer(cl);
		}

		void BundleCore::addConnection(const dtn::core::Node &n)
		{
			_connectionmanager.addConnection(n);
		}

		const std::set<dtn::core::Node> BundleCore::getNeighbors()
		{
			return _connectionmanager.getNeighbors();
		}

		void BundleCore::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				/**
				 * evaluate the current local time
				 */
				if (dtn::utils::Clock::quality == 0)
				{
					if (dtn::utils::Clock::getTime() > 0)
					{
						dtn::utils::Clock::quality = 1;
						IBRCOMMON_LOGGER(warning) << "The local clock seems to be okay again. Expiration enabled." << IBRCOMMON_LOGGER_ENDL;
					}
				}

			} catch (std::bad_cast ex) {

			}

			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);
				const dtn::data::MetaBundle &meta = queued.bundle;

				if (meta.destination == local)
				{
					// if the delivered variable is still false at the end of this block.
					// we send a not delivered report.
					bool delivered = false;

					// process this bundle locally
					dtn::data::Bundle bundle = getStorage().get(meta);

					try {
						// check for a custody signal
						dtn::data::CustodySignalBlock custody = bundle.getBlock<dtn::data::CustodySignalBlock>();
						dtn::data::BundleID id(custody._source, custody._bundle_timestamp.getValue(), custody._bundle_sequence.getValue(), (custody._fragment_length.getValue() > 0), custody._fragment_offset.getValue());
						getStorage().releaseCustody(id);

						IBRCOMMON_LOGGER_DEBUG(5) << "custody released for " << bundle.toString() << IBRCOMMON_LOGGER_ENDL;

						delivered = true;
					} catch (dtn::data::Bundle::NoSuchBlockFoundException) {
						// no custody signal available
					}

					if (delivered)
					{
						// gen a report
						dtn::core::BundleEvent::raise(meta, BUNDLE_DELIVERED);
					}
					else
					{
						// gen a report
						dtn::core::BundleEvent::raise(meta, BUNDLE_DELETED, StatusReportBlock::DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE);
					}

					// delete the bundle
					getStorage().remove(meta);
				}
			} catch (std::bad_cast ex) {

			}
		}

		void BundleCore::validate(const dtn::data::PrimaryBlock &p) const throw (dtn::data::Validator::RejectedException)
		{
			/*
			 *
			 * TODO: reject a bundle if...
			 * ... it is expired
			 * ... already in the storage
			 * ... a fragment of an already received bundle in the storage
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */

			// if we do not forward bundles
			if (!BundleCore::forwarding)
			{
				if (!p._destination.sameHost(BundleCore::local))
				{
					// ... we reject all non-local bundles.
					IBRCOMMON_LOGGER(warning) << "non-local bundle rejected: " << p.toString() << IBRCOMMON_LOGGER_ENDL;
					throw dtn::data::Validator::RejectedException("bundle is not local");
				}
			}

			// check if the bundle is expired
			if (p.isExpired())
			{
				IBRCOMMON_LOGGER(warning) << "bundle rejected: bundle has expired (" << p.toString() << ")" << IBRCOMMON_LOGGER_ENDL;
				throw dtn::data::Validator::RejectedException("bundle is expired");
			}
		}

		void BundleCore::validate(const dtn::data::Block&, const size_t size) const throw (dtn::data::Validator::RejectedException)
		{
			/*
			 *
			 * TODO: reject a block if
			 * ... it exceeds the payload limit
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */

			// check for the size of the block
			if ((BundleCore::blocksizelimit > 0) && (size > BundleCore::blocksizelimit))
			{
				IBRCOMMON_LOGGER(warning) << "bundle rejected: block size of " << size << " is too big" << IBRCOMMON_LOGGER_ENDL;
				throw dtn::data::Validator::RejectedException("block size is too big");
			}
		}

		void BundleCore::validate(const dtn::data::Bundle&) const throw (dtn::data::Validator::RejectedException)
		{
			/*
			 *
			 * TODO: reject a bundle if
			 * ... the security checks (DTNSEC) failed
			 * ... a checksum mismatch is detected (CRC)
			 *
			 * throw dtn::data::DefaultDeserializer::RejectedException();
			 *
			 */
		}

		const std::string BundleCore::getName() const
		{
			return "BundleCore";
		}
	}
}
