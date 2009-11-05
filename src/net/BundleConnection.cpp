/*
 * BundleConnection.cpp
 *
 *  Created on: 26.10.2009
 *      Author: morgenro
 */

#include "net/BundleConnection.h"
#include "core/EventSwitch.h"
#include "core/BundleEvent.h"
#include "core/RouteEvent.h"

namespace dtn
{
	namespace net
	{
		BundleConnection::BundleReceiver::BundleReceiver(BundleConnection &connection)
		 :  _running(true), _connection(connection)
		{
		}

		BundleConnection::BundleReceiver::~BundleReceiver()
		{
			join();
		}

		void BundleConnection::BundleReceiver::run()
		{
			try {
				while (_running)
				{
					dtn::data::Bundle bundle;
					_connection.read(bundle);

					// raise default bundle received event
					dtn::core::EventSwitch::raiseEvent( new dtn::core::BundleEvent(bundle, dtn::core::BUNDLE_RECEIVED) );
					dtn::core::EventSwitch::raiseEvent( new dtn::core::RouteEvent(bundle, dtn::core::ROUTE_PROCESS_BUNDLE) );

					yield();
				}
			} catch (dtn::exceptions::IOException ex) {
				_running = false;
			}
		}

		void BundleConnection::BundleReceiver::shutdown()
		{
			_running = false;
		}

		BundleConnection::BundleConnection()
		 : _receiver(*this)
		{
		}

		BundleConnection::~BundleConnection()
		{
		}

		void BundleConnection::initialize()
		{
			_receiver.start();
		}

		void BundleConnection::shutdown()
		{
			_receiver.shutdown();
		}

		void BundleConnection::waitFor()
		{
			_receiver.waitFor();
		}
	}
}
