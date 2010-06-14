/*
 * AbstractWorker.cpp
 *
 *  Created on: 30.10.2009
 *      Author: morgenro
 */

#include "core/AbstractWorker.h"
#include "core/BundleCore.h"
#include "net/BundleReceivedEvent.h"
#include "ibrcommon/thread/MutexLock.h"
#include <typeinfo>

namespace dtn
{
	namespace core
	{
		AbstractWorker::AbstractWorkerAsync::AbstractWorkerAsync(AbstractWorker &worker)
		 : _worker(worker), _running(true)
		{
			bindEvent(dtn::net::BundleReceivedEvent::className);
		}

		AbstractWorker::AbstractWorkerAsync::~AbstractWorkerAsync()
		{
			unbindEvent(dtn::net::BundleReceivedEvent::className);
			shutdown();
		}

		void AbstractWorker::AbstractWorkerAsync::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::net::BundleReceivedEvent &received = dynamic_cast<const dtn::net::BundleReceivedEvent&>(*evt);

				if (received.getBundle().destination == _worker._eid)
				{
					ibrcommon::MutexLock l(_receive_cond);
					_receive_bundles.push(received.getBundle());
					_receive_cond.signal(true);
				}
			} catch (std::bad_cast ex) {

			}
		}

		void AbstractWorker::AbstractWorkerAsync::shutdown()
		{
			{
				ibrcommon::MutexLock l(_receive_cond);
				_running = false;
				_receive_cond.signal(true);
			}

			join();
		}

		void AbstractWorker::AbstractWorkerAsync::run()
		{
			BundleStorage &storage = BundleCore::getInstance().getStorage();
			dtn::data::Bundle b;

			while (_running)
			{
				try {
					b = storage.get( _worker._eid );
					storage.remove(b);
				} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
					{
						ibrcommon::MutexLock l(_receive_cond);
						while (_receive_bundles.empty())
						{
							if (!_running) return;
							_receive_cond.wait();
						}

						try {
							b = storage.get( _receive_bundles.front() );
							storage.remove(b);
						} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {

						}

						_receive_bundles.pop();
					}
				}
				_worker.callbackBundleReceived(b);
			}
		}

		AbstractWorker::AbstractWorker() : _thread(*this)
		{
		};

		void AbstractWorker::initialize(const string uri, bool async)
		{
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

//		dtn::data::Bundle AbstractWorker::receive()
//		{
//			BundleStorage &storage = BundleCore::getInstance().getStorage();
//			dtn::data::Bundle b = storage.get( _eid );
//			storage.remove(b);
//			return b;
//		}
	}
}
