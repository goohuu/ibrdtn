#include "core/BundleCore.h"
#include "core/BundleSchedule.h"
#include "core/AbstractWorker.h"
#include "core/CustodyManager.h"
#include "core/EventSwitch.h"
#include "core/StorageEvent.h"
#include "core/RouteEvent.h"
#include "core/CustodyEvent.h"
#include "core/BundleEvent.h"
#include "core/TimeEvent.h"

#include "data/Bundle.h"
#include "data/Exceptions.h"
#include "data/AdministrativeBlock.h"
#include "data/PrimaryFlags.h"
#include "data/PayloadBlockFactory.h"
#include "data/EID.h"

#include "utils/Utils.h"
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
		BundleCore& BundleCore::getInstance(string eid)
		{
			BundleCore &core = BundleCore::getInstance();
			core.setLocalEID(eid);
			return core;
		}

		BundleCore& BundleCore::getInstance()
		{
			static BundleCore instance;
			return instance;
		}

		void BundleCore::setLocalEID(string eid)
		{
			m_localeid = eid;
		}

		BundleCore::BundleCore()
		 : Service("BundleCore"), m_clayer(NULL), m_localeid("dtn:none"), m_dtntime(0)
		{
			// register me for events
			EventSwitch::registerEventReceiver( RouteEvent::className, this );
			EventSwitch::registerEventReceiver( CustodyEvent::className, this );
			EventSwitch::registerEventReceiver( BundleEvent::className, this );

			// start the custody manager
			m_cm.start();
		}

		BundleCore::~BundleCore()
		{
			EventSwitch::unregisterEventReceiver( RouteEvent::className, this );
			EventSwitch::unregisterEventReceiver( CustodyEvent::className, this );
			EventSwitch::unregisterEventReceiver( BundleEvent::className, this );

			// stop the custody manager
			m_cm.abort();
		}

		void BundleCore::tick()
		{
			unsigned int dtntime = BundleFactory::getDTNTime();
			if (m_dtntime != dtntime)
			{
				EventSwitch::raiseEvent( new TimeEvent(dtntime, TIME_SECOND_TICK) );
				m_dtntime = dtntime;
			}
			usleep(5000);
		}

		void BundleCore::setConvergenceLayer(ConvergenceLayer *cl)
		{
			cl->setBundleReceiver(this);
			m_clayer = cl;
		}

		ConvergenceLayer* BundleCore::getConvergenceLayer()
		{
			return m_clayer;
		}

		CustodyManager& BundleCore::getCustodyManager()
		{
			return m_cm;
		}

		void BundleCore::raiseEvent(const Event *evt)
		{
			const BundleEvent *bundleevent = dynamic_cast<const BundleEvent*>(evt);

			if (bundleevent != NULL)
			{
				const Bundle &b = bundleevent->getBundle();

				switch (bundleevent->getAction())
				{
				case BUNDLE_RECEIVED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_RECEPTION) )
					{
						Bundle bundle = createStatusReport(b, RECEIPT_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;
				case BUNDLE_DELETED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_DELETION) )
					{
						Bundle bundle = createStatusReport(b, DELETION_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;

				case BUNDLE_FORWARDED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_FORWARDING) )
					{
						Bundle bundle = createStatusReport(b, FORWARDING_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;

				case BUNDLE_DELIVERED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY) )
					{
						Bundle bundle = createStatusReport(b, DELIVERY_OF_BUNDLE, bundleevent->getReason());
						transmit( bundle );
					}
					break;

				case BUNDLE_CUSTODY_ACCEPTED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE) )
					{
						Bundle bundle = createStatusReport(b, CUSTODY_ACCEPTANCE_OF_BUNDLE, bundleevent->getReason());
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

		void BundleCore::transmit(const Node &n, const Bundle &b)
		{
			// get convergence layer to reach the node
			ConvergenceLayer *clayer = n.getConvergenceLayer();

			if (b.getPrimaryFlags().isCustodyRequested())
			{
				if (b.getCustodian() == m_localeid)
				{
					// set custody timer
					m_cm.setTimer(b, 5, 1);
				}
				else
				{
					// here i need a copy
					Bundle b_copy = b;

					// set me as custody
					b_copy.setCustodian(m_localeid);

					// accept custody
					EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_ACCEPT ) );

					// set custody timer
					m_cm.setTimer(b_copy, 5, 1);

					// send the bundle
					clayer->transmit(b_copy, n);

					// bundle forwarded event
					EventSwitch::raiseEvent( new BundleEvent( b_copy, BUNDLE_FORWARDED ) );

					return;
				}
			}

			// send the bundle
			clayer->transmit(b, n);

			// bundle forwarded event
			EventSwitch::raiseEvent( new BundleEvent( b, BUNDLE_FORWARDED ) );
		}

		void BundleCore::deliver(const Bundle &b)
		{
			PrimaryFlags flags = b.getPrimaryFlags();

			if (flags.isFragment())
			{
				EventSwitch::raiseEvent( new StorageEvent(b) );
				return;
			}

			EID eid(b.getDestination());

			if (eid.hasApplication())
			{
				AbstractWorker *worker = m_worker[ b.getDestination() ];
				if (worker != NULL)
				{
					switch ( worker->callbackBundleReceived( b ) )
					{
						case BUNDLE_ACCEPTED:
							// accept custody
							EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_ACCEPT ) );

							// raise delivered event
							EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELIVERED) );
						break;

						default:
							// reject custody
							EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );

							// raise deleted event
							EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
						break;
					}
				}
				else
				{
					// reject custody
					EventSwitch::raiseEvent( new CustodyEvent( b, CUSTODY_REJECT ) );

					// raise deleted event
					EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
				}
			}
			else
			{
				// process bundles for the daemon (e.g. custody signals)
				Block *block = b.getPayloadBlock();
				CustodySignalBlock *signal = dynamic_cast<CustodySignalBlock*>( block );

				if ( signal != NULL )
				{
					try {
						// remove the timer for this bundle
						Bundle bundle = m_cm.removeTimer( *signal );

						// and requeue the bundle for later delivery
						BundleSchedule sched(bundle, BundleFactory::getDTNTime() + 60, EID(bundle.getDestination()).getNodeEID());
						EventSwitch::raiseEvent( new StorageEvent( sched ) );
					} catch (NoTimerFoundException ex) {

					}
				}
			}
		}

		Bundle BundleCore::createStatusReport(const Bundle &b, StatusReportType type, StatusReportReasonCode reason)
		{
			// create a new bundle
			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *bundle = fac.newBundle();

			// create a new statusreport block
			StatusReportBlock *report = PayloadBlockFactory::newStatusReportBlock();

			// add the report to the bundle
			bundle->appendBlock(report);

			// get the flags and set the status flag
			ProcessingFlags flags = report->getStatusFlags();
			flags.setFlag(type, true);
			report->setStatusFlags(flags);

			// set the reason code
			report->setReasonCode(ProcessingFlags(reason));

			switch (type)
			{
				case RECEIPT_OF_BUNDLE:
					report->setTimeOfReceipt( BundleFactory::getDTNTime() );
				break;

				case CUSTODY_ACCEPTANCE_OF_BUNDLE:
					report->setTimeOfCustodyAcceptance( BundleFactory::getDTNTime() );
				break;

				case FORWARDING_OF_BUNDLE:
					report->setTimeOfForwarding( BundleFactory::getDTNTime() );
				break;

				case DELIVERY_OF_BUNDLE:
					report->setTimeOfDelivery( BundleFactory::getDTNTime() );
				break;

				case DELETION_OF_BUNDLE:
					report->setTimeOfDeletion( BundleFactory::getDTNTime() );
				break;

				default:

				break;
			}

			// set source and destination
			bundle->setSource(m_localeid);
			bundle->setDestination(b.getReportTo());

			// set bundle parameter
			report->setMatch(b);

			Bundle ret = *bundle;
			delete bundle;

			return ret;
		}

		Bundle BundleCore::createCustodySignal(const Bundle &b, bool accepted)
		{
			// create a new bundle
			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *bundle = fac.newBundle();

			// create a new CustodySignalBlock block
			CustodySignalBlock *custody = PayloadBlockFactory::newCustodySignalBlock(accepted);

			// add the block to the bundle
			bundle->appendBlock(custody);

			// set source and destination
			bundle->setSource(m_localeid);
			bundle->setDestination(b.getCustodian());

			// set bundle parameter
			custody->setMatch(b);

			Bundle ret = *bundle;
			delete bundle;

			return ret;
		}

		void BundleCore::received(const ConvergenceLayer &cl, const Bundle &b)
		{
			EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_RECEIVED) );
			EventSwitch::raiseEvent( new RouteEvent(b, ROUTE_PROCESS_BUNDLE) );
		}

		void BundleCore::registerSubNode(string eid, AbstractWorker *node)
		{
			m_worker[m_localeid + eid] = node;
		}

		void BundleCore::unregisterSubNode(string eid)
		{
			m_worker.erase(m_localeid + eid);
		}

		string BundleCore::getLocalURI() const
		{
			return m_localeid;
		}
	}
}
