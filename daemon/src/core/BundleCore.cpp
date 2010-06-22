#include "core/BundleCore.h"
#include "core/GlobalEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"

#include <ibrcommon/data/BLOB.h>
#include <ibrdtn/data/MetaBundle.h>
#include <ibrdtn/data/Exceptions.h>
#include <ibrdtn/data/EID.h>
#include <ibrdtn/utils/Utils.h>
#include <ibrcommon/Logger.h>

#include "limits.h"
#include <iostream>
#include <typeinfo>

using namespace dtn::data;
using namespace dtn::utils;
using namespace std;

namespace dtn
{
	namespace core
	{
		dtn::data::EID BundleCore::local;

		BundleCore& BundleCore::getInstance()
		{
			static BundleCore instance;
			return instance;
		}

		BundleCore::BundleCore()
		 : _clock(1), _storage(NULL)
		{
			bindEvent(dtn::net::BundleReceivedEvent::className);
		}

		BundleCore::~BundleCore()
		{
			unbindEvent(dtn::net::BundleReceivedEvent::className);
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

		Clock& BundleCore::getClock()
		{
			return _clock;
		}

		void BundleCore::transferTo(const dtn::data::EID &destination, dtn::data::Bundle &bundle)
		{
			try {
				_connectionmanager.queue(destination, bundle);
			} catch (dtn::net::NeighborNotAvailableException ex) {
				// signal interruption of the transfer
				dtn::routing::RequeueBundleEvent::raise(destination, bundle);
			} catch (dtn::net::ConnectionNotAvailableException ex) {
				// signal interruption of the transfer
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

		const std::list<dtn::core::Node> BundleCore::getNeighbors()
		{
			return _connectionmanager.getNeighbors();
		}

		void BundleCore::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::net::BundleReceivedEvent &received = dynamic_cast<const dtn::net::BundleReceivedEvent&>(*evt);
				const dtn::data::MetaBundle &meta = received.getBundle();

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
	}
}
