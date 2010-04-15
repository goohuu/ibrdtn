#include "core/BundleCore.h"
#include "core/CustodyManager.h"
#include "core/CustodyEvent.h"
#include "core/GlobalEvent.h"

#include "ibrcommon/data/BLOB.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/EID.h"

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
			// start the custody manager
			_cm.start();

			// start a clock
			_clock.start();
		}

		BundleCore::~BundleCore()
		{
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
			_connectionmanager.send(destination, bundle);
		}

		void BundleCore::addConvergenceLayer(dtn::net::ConvergenceLayer *cl)
		{
			_connectionmanager.addConvergenceLayer(cl);
		}

		void BundleCore::addConnection(const dtn::core::Node &n)
		{
			_connectionmanager.addConnection(n);
		}

//		void BundleCore::transmit(const Node &n, const Bundle &b)
//		{
//			// get convergence layer to reach the node
//			dtn::net::ConvergenceLayer *clayer = n.getConvergenceLayer();
//
//			if (b._procflags & Bundle::CUSTODY_REQUESTED )
//			{
//				if (b._custodian == m_localeid)
//				{
//					// set custody timer
//					m_cm.setTimer(b, n.getRoundTripTime(), 1);
//				}
//				else
//				{
//					// here i need a copy
//					Bundle b_copy = b;
//
//					// set me as custody
//					b_copy._custodian = m_localeid;
//
//					// accept custody
//					EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_ACCEPT ) );
//
//					// set custody timer
//					m_cm.setTimer(b_copy, n.getRoundTripTime(), 1);
//
//					// send the bundle
//					clayer->transmit(b_copy, n);
//
//					// bundle forwarded event
//					EventSwitch::raiseEvent( new BundleEvent( b_copy, BUNDLE_FORWARDED ) );
//
//					return;
//				}
//			}
//
//			// send the bundle
//			clayer->transmit(b, n);
//
//			// bundle forwarded event
//			EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_FORWARDED ) );
//		}

//		void BundleCore::deliver(const Bundle &b)
//		{
//			if (b._procflags & Bundle::FRAGMENT)
//			{
//				// TODO: put a fragment into the storage!
//				return;
//			}
//
//			if (b._destination.hasApplication())
//			{
//				AbstractWorker *worker = getSubNode(b._destination);
//
//				if (worker != NULL)
//				{
//					ibrcommon::MutexLock l(*worker);
//
//					switch ( worker->callbackBundleReceived( b ) )
//					{
//						case dtn::net::BUNDLE_ACCEPTED:
//							// accept custody
//							EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_ACCEPT ) );
//
//							// raise delivered event
//							EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELIVERED) );
//						break;
//
//						default:
//							// reject custody
//							EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
//
//							// raise deleted event
//							EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
//						break;
//					}
//				}
//				else
//				{
//					// reject custody
//					EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );
//
//					// raise deleted event
//					EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
//				}
//			}
//			else
//			{
//				// process bundles for the daemon (e.g. custody signals)
//				CustodySignalBlock *signal = Utils::getCustodySignalBlock(b);
//
//				if ( signal != NULL )
//				{
//					try {
//						// remove the timer for this bundle
//						Bundle bundle = m_cm.removeTimer( *signal );
//
//						// and requeue the bundle for later delivery
//						//BundleSchedule sched(bundle, Utils::get_current_dtn_time() + 60, EID(bundle._destination.getNodeEID()));
//
//						// TODO: put the bundle into the storage for a new attempt later.
//
//					} catch (NoTimerFoundException ex) {
//
//					}
//				}
//			}
//		}
	}
}
