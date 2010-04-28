#include "core/BundleCore.h"
#include "core/CustodyManager.h"
#include "core/CustodyEvent.h"
#include "core/GlobalEvent.h"

#include "ibrcommon/data/BLOB.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/EID.h"
#include "routing/RequeueBundleEvent.h"

#include "ibrdtn/utils/Utils.h"
#include "limits.h"
#include <iostream>

using namespace dtn::exceptions;
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
		}

		BundleCore::~BundleCore()
		{
		}

		void BundleCore::componentUp()
		{
			_connectionmanager.initialize();
			_clock.initialize();

			// start the custody manager
			_cm.start();

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

		CustodyManager& BundleCore::getCustodyManager()
		{
			return _cm;
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
	}
}
