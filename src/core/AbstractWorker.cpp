/*
 * AbstractWorker.cpp
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#include "core/AbstractWorker.h"
#include "core/BundleCore.h"
#include "net/BundleReceivedEvent.h"


namespace dtn
{
	namespace core
	{
		AbstractWorker::AbstractWorkerAsync::AbstractWorkerAsync(AbstractWorker &worker)
		 : _worker(worker), _running(true)
		{

		}

		AbstractWorker::AbstractWorkerAsync::~AbstractWorkerAsync()
		{
			shutdown();
		}

		void AbstractWorker::AbstractWorkerAsync::shutdown()
		{
			if (!_running)
			{
				join();
				return;
			}

			_running = false;

			BundleStorage &storage = BundleCore::getInstance().getStorage();
			storage.unblock(_worker._eid);

			join();
		}

		void AbstractWorker::AbstractWorkerAsync::run()
		{
			while (_running)
			{
				try {
					dtn::data::Bundle b = _worker.receive();
					_worker.callbackBundleReceived(b);
				} catch (dtn::exceptions::NoBundleFoundException ex) {
					return;
				}
			}
		}

		AbstractWorker::AbstractWorker() : _thread(*this)
		{
		};

		void AbstractWorker::initialize(const string uri, bool async)
		{
			BundleCore &core = BundleCore::getInstance();
			_eid = BundleCore::local + uri;

			if (async) _thread.start();
		}

		AbstractWorker::~AbstractWorker()
		{
			shutdown();
		};

		void AbstractWorker::shutdown()
		{
			// wait for the async thread
			_thread.shutdown();
		}

		const EID AbstractWorker::getWorkerURI() const
		{
			return _eid;
		}

		void AbstractWorker::transmit(const Bundle &bundle)
		{
			dtn::net::BundleReceivedEvent::raise(_eid, bundle);
		}

		dtn::data::Bundle AbstractWorker::receive()
		{
			BundleStorage &storage = BundleCore::getInstance().getStorage();
			dtn::data::Bundle b = storage.get(_eid);
			storage.remove(b);
			return b;
		}
	}
}
