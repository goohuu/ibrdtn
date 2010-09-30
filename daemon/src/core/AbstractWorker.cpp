/*
 * AbstractWorker.cpp
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#include "core/AbstractWorker.h"
#include "core/BundleCore.h"
#include "routing/QueueBundleEvent.h"
#include "core/BundleGeneratedEvent.h"
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/Logger.h>
#include <typeinfo>

namespace dtn
{
	namespace core
	{
		AbstractWorker::AbstractWorkerAsync::AbstractWorkerAsync(AbstractWorker &worker)
		 : _worker(worker), _running(true)
		{
			bindEvent(dtn::routing::QueueBundleEvent::className);
		}

		AbstractWorker::AbstractWorkerAsync::~AbstractWorkerAsync()
		{
			unbindEvent(dtn::routing::QueueBundleEvent::className);
			shutdown();
		}

		void AbstractWorker::AbstractWorkerAsync::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);

				if (queued.bundle.destination == _worker._eid)
				{
					_receive_bundles.push(queued.bundle);
				}
			} catch (std::bad_cast ex) {

			}
		}

		void AbstractWorker::AbstractWorkerAsync::shutdown()
		{
			_running = false;
			_receive_bundles.abort();

			join();
		}

		void AbstractWorker::AbstractWorkerAsync::run()
		{
			BundleStorage &storage = BundleCore::getInstance().getStorage();

			while (_running)
			{
				dtn::data::Bundle b;

				try {
					b = storage.get( _worker._eid );
					storage.remove(b);
					_worker.callbackBundleReceived(b);
				}
				catch (dtn::core::BundleStorage::NoBundleFoundException ex)
				{
					dtn::data::BundleID id = _receive_bundles.getnpop(true);

					try {
						b = storage.get( _receive_bundles.front() );
						storage.remove(b);
						_worker.callbackBundleReceived(b);
					} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {

					}
				}
			}
		}

		AbstractWorker::AbstractWorker() : _thread(*this)
		{
		};

		void AbstractWorker::initialize(const string uri, bool async)
		{
			_eid = BundleCore::local + uri;

			try {
				if (async) _thread.start();
			} catch (const ibrcommon::ThreadException &ex) {
				IBRCOMMON_LOGGER(error) << "failed to start thread in AbstractWorker" << IBRCOMMON_LOGGER_ENDL;
			}
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
			dtn::core::BundleGeneratedEvent::raise(bundle);
		}
	}
}
