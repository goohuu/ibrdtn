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
#include "core/BundleEvent.h"
#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#endif
#ifdef WITH_COMPRESSION
#include <ibrdtn/data/CompressedPayloadBlock.h>
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

				// check for bundle destination
				if (queued.bundle.destination == _worker._eid)
				{
					_receive_bundles.push(queued.bundle);
					return;
				}

				// if the bundle is a singleton, stop here
				if (queued.bundle.procflags & dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON) return;

				// check for subscribed groups
				if (_worker._groups.find(queued.bundle.destination) != _worker._groups.end())
				{
					_receive_bundles.push(queued.bundle);
					return;
				}
			} catch (const std::bad_cast&) { }
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

						// raise bundle event
						dtn::core::BundleEvent::raise(b, BUNDLE_DELIVERED);

						// remove the bundle from the storage
						storage.remove( id );
					} catch (const dtn::core::BundleStorage::NoBundleFoundException&) { };

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
			// process the bundle block (security, compression, ...)
			dtn::core::BundleCore::processBlocks(bundle);
		}

		AbstractWorker::AbstractWorker() : _thread(*this)
		{
		};

		void AbstractWorker::subscribe(const dtn::data::EID &endpoint)
		{
			_groups.insert(endpoint);
		}

		void AbstractWorker::unsubscribe(const dtn::data::EID &endpoint)
		{
			_groups.erase(endpoint);
		}

		void AbstractWorker::initialize(const string uri, const size_t cbhe, bool async)
		{
			if (BundleCore::local.getScheme() == dtn::data::EID::CBHE_SCHEME)
			{
				std::stringstream cbhe_id; cbhe_id << cbhe;
				_eid = BundleCore::local + "." + cbhe_id.str();
			}
			else
			{
				_eid = BundleCore::local + uri;
			}

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
