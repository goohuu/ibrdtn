/*
 * AbstractWorker.cpp
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#include "config.h"
#include "core/AbstractWorker.h"
#include "core/BundleCore.h"
#include "routing/QueueBundleEvent.h"
#include "core/BundleGeneratedEvent.h"
#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#endif
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

			try {
				while (_running)
				{
					dtn::data::BundleID id = _receive_bundles.getnpop(true);

					try {
						dtn::data::Bundle b = storage.get( id );
						prepareBundle(b);
						_worker.callbackBundleReceived( b );
						storage.remove( id );
					} catch (dtn::core::BundleStorage::NoBundleFoundException ex) { };

					yield();
				}
			} catch (const ibrcommon::QueueUnblockedException&) {
				// queue was aborted by another call
			}
		}

		bool AbstractWorker::AbstractWorkerAsync::__cancellation()
		{
			// cancel the main thread in here
			_receive_bundles.abort();

			// return true, to signal that no further cancel (the hardway) is needed
			return true;
		}

		void AbstractWorker::AbstractWorkerAsync::prepareBundle(dtn::data::Bundle &bundle) const
		{
#ifdef WITH_BUNDLE_SECURITY
			// try to decrypt the bundle
			try {
				dtn::security::SecurityManager::getInstance().decrypt(bundle);
			} catch (const dtn::security::SecurityManager::KeyMissingException&) {
				// decrypt needed, but no key is available
				IBRCOMMON_LOGGER(warning) << "No key available for decrypt bundle." << IBRCOMMON_LOGGER_ENDL;
			} catch (const dtn::security::SecurityManager::DecryptException &ex) {
				// decrypt failed
				IBRCOMMON_LOGGER(warning) << "Decryption of bundle failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}
#endif
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
				IBRCOMMON_LOGGER(error) << "failed to start thread in AbstractWorker\n" << ex.what() << IBRCOMMON_LOGGER_ENDL;
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
