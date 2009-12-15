#include "core/BundleCore.h"
#include "core/CustodyManager.h"
#include "core/EventSwitch.h"
#include "core/RouteEvent.h"
#include "core/CustodyEvent.h"
#include "core/NodeEvent.h"
#include "core/BundleEvent.h"
#include "core/TimeEvent.h"
#include "core/GlobalEvent.h"

#include "ibrcommon/data/BLOBManager.h"
#include "ibrcommon/data/BLOBReference.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/EID.h"

#include "ibrdtn/utils/Utils.h"
#include "ibrcommon/net/tcpserver.h"
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
		 : _clock(1), _shutdown(false), _storage(NULL)
		{
			// register me for events
			EventSwitch::registerEventReceiver( NodeEvent::className, this );
			EventSwitch::registerEventReceiver( BundleEvent::className, this );
			EventSwitch::registerEventReceiver( TimeEvent::className, this );

			// start the custody manager
			m_cm.start();

			// start a clock
			_clock.start();
		}

		BundleCore::~BundleCore()
		{
			EventSwitch::unregisterEventReceiver( NodeEvent::className, this );
			EventSwitch::unregisterEventReceiver( BundleEvent::className, this );
			EventSwitch::unregisterEventReceiver( TimeEvent::className, this );
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
			return m_cm;
		}

		void BundleCore::raiseEvent(const Event *evt)
		{
			const BundleEvent *bundleevent = dynamic_cast<const BundleEvent*>(evt);
			const NodeEvent *nodeevent = dynamic_cast<const NodeEvent*>(evt);
			const TimeEvent *timeevent = dynamic_cast<const TimeEvent*>(evt);
			const GlobalEvent *globalevent = dynamic_cast<const GlobalEvent*>(evt);

			if (globalevent != NULL)
			{
				//ibrcommon::MutexLock l(_register_action);
				_shutdown = true;
				//_register_action.signal(true);
			}
			else if (timeevent != NULL)
			{
				if (timeevent->getAction() == TIME_SECOND_TICK)
				{
					check_discovered();
				}
			}
			else if (nodeevent != NULL)
			{
				Node n = nodeevent->getNode();

				if (nodeevent->getAction() == NODE_INFO_UPDATED)
				{
					discovered(n);
				}
			}
			else if (bundleevent != NULL)
			{
				const Bundle &b = bundleevent->getBundle();

				switch (bundleevent->getAction())
				{
				case BUNDLE_RECEIVED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_RECEPTION )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::RECEIPT_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;
				case BUNDLE_DELETED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_DELETION )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::DELETION_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;

				case BUNDLE_FORWARDED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_FORWARDING )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::FORWARDING_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;

				case BUNDLE_DELIVERED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_DELIVERY )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::DELIVERY_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;

				case BUNDLE_CUSTODY_ACCEPTED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::CUSTODY_ACCEPTANCE_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;
				}
			}
		}

		void BundleCore::transmit(const Bundle &b)
		{
			EventSwitch::raiseEvent( new RouteEvent( b, ROUTE_PROCESS_BUNDLE ) );
		}

		void BundleCore::transmit(const dtn::data::EID &eid, const Bundle &b)
		{
			try {
				// send the bundle with the ConnectionManager
				ConnectionManager::send(eid, b);
			} catch (ibrcommon::tcpserver::SocketException ex) {
				// connection not possible
				// TODO: requeue!
				throw dtn::exceptions::NotImplementedException("connection not possible, requeue!");
			}
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

		Bundle BundleCore::createStatusReport(const Bundle &b, StatusReportBlock::TYPE type, StatusReportBlock::REASON_CODE reason)
		{
			// create a new bundle
			Bundle bundle;

			// create a new statusreport block
			StatusReportBlock *report = new StatusReportBlock();

			// get the flags and set the status flag
			if (!(report->_status & type)) report->_status += type;

			// set the reason code
			if (!(report->_reasoncode & reason)) report->_reasoncode += reason;

			switch (type)
			{
				case StatusReportBlock::RECEIPT_OF_BUNDLE:
					report->_timeof_receipt.set();
				break;

				case StatusReportBlock::CUSTODY_ACCEPTANCE_OF_BUNDLE:
					report->_timeof_custodyaccept.set();
				break;

				case StatusReportBlock::FORWARDING_OF_BUNDLE:
					report->_timeof_forwarding.set();
				break;

				case StatusReportBlock::DELIVERY_OF_BUNDLE:
					report->_timeof_delivery.set();
				break;

				case StatusReportBlock::DELETION_OF_BUNDLE:
					report->_timeof_deletion.set();
				break;

				default:

				break;
			}

			// set source and destination
			bundle._source = BundleCore::local;
			bundle._destination = b._reportto;

			// set bundle parameter
			if (b._procflags & Bundle::FRAGMENT)
			{
				report->_fragment_offset = b._fragmentoffset;
				report->_fragment_length = b._appdatalength;

				if (!(report->_admfield & 1)) report->_admfield += 1;
			}

			report->_bundle_timestamp = b._timestamp;
			report->_bundle_sequence = b._sequencenumber;
			report->_source = b._source;

			// commit the data
			report->commit();

			// add the report to the bundle
			bundle.addBlock(report);

			return bundle;
		}

		Bundle BundleCore::createCustodySignal(const Bundle &b, bool accepted)
		{
			// create a new bundle
			Bundle bundle;

			// create a new statusreport block
			CustodySignalBlock *custody = new CustodySignalBlock();

			// set accepted
			if (accepted)
			{
				if (!(custody->_status & 1)) custody->_status += 1;
			}

			// set source and destination
			custody->match(b);

			// commit the data
			custody->commit();

			// add the report to the bundle
			bundle.addBlock(custody);

			return bundle;
		}
	}
}
