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
						Bundle bundle = createStatusReport(b, RECEIPT_OF_BUNDLE);
						transmit( bundle );
					}
					break;
				case BUNDLE_DELETED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_DELETION) )
					{
						Bundle bundle = createStatusReport(b, DELETION_OF_BUNDLE);
						transmit( bundle );
					}
					break;

				case BUNDLE_FORWARDED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_FORWARDING) )
					{
						Bundle bundle = createStatusReport(b, FORWARDING_OF_BUNDLE);
						transmit( bundle );
					}
					break;

				case BUNDLE_DELIVERED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY) )
					{
						Bundle bundle = createStatusReport(b, DELIVERY_OF_BUNDLE);
						transmit( bundle );
					}
					break;

				case BUNDLE_CUSTODY_ACCEPTED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE) )
					{
						Bundle bundle = createStatusReport(b, CUSTODY_ACCEPTANCE_OF_BUNDLE);
						transmit( bundle );
					}
					break;
				}
			}
		}

		void BundleCore::transmit(Bundle &b)
		{
			EventSwitch::raiseEvent( new RouteEvent( b, ROUTE_PROCESS_BUNDLE ) );
		}

//		TransmitReport BundleCore::transmit(Bundle &b)
//		{
//			// check all block of the bundle for block flag actions
//			list<Block*> blocks = b.getBlocks();
//			list<Block*>::const_iterator iter = blocks.begin();
//
//			while (iter != blocks.end())
//			{
//				Block *block = (*iter);
//				BlockFlags flags = block->getBlockFlags();
//
//				if ( !block->isProcessed() )
//				{
//					// if forwarded without processed, mark it!
//					flags.setFlag(FORWARDED_WITHOUT_PROCESSED, true);
//					block->setBlockFlags(flags);
//					block->updateBlockSize();
//				}
//
//				iter++;
//			}
//
//			try {
//				// Wurde Custody angefordert?
//				PrimaryFlags flags( b.getInteger(PROCFLAGS) );
//
//				if (flags.isCustodyRequested())
//				{
//					// Setze Custody EID auf die eigene EID
//					b.setCustodian( m_localeid );
//				}
//
//				// find a next route for the bundle
//				EventSwitch::raiseEvent( new RouteEvent(b, ROUTE_FIND_SCHEDULE) );
//
//				return BUNDLE_ACCEPTED;
//			} catch (NoSpaceLeftException ex) {
//				// Bündel verwerfen
//				EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
//				return UNKNOWN;
//			} catch (BundleExpiredException ex) {
//				return UNKNOWN;
//			} catch (NoScheduleFoundException ex) {
//				return NO_ROUTE_FOUND;
//			};
//		}

		void BundleCore::transmit(const Node &n, const Bundle &b)
		{
			// get convergence layer to reach the node
			ConvergenceLayer *clayer = n.getConvergenceLayer();

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
					// remove the timer for this bundle
					m_cm.removeTimer( signal );
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

//		void BundleCore::transmitCustody(bool accept, const Bundle &b)
//		{
//			PrimaryFlags flags = b.getPrimaryFlags();
//
//			// Senden an
//			string eid = b.getCustodian();
//
//			// Custody-Acceptance Event
//			EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_CUSTODY_ACCEPTED) );
//
//			// Erstelle ein CustodySignal
//			Bundle bundle = createCustodySignal(b, accept);
//
//			// Absender setzen
//			bundle.setSource( m_localeid );
//
//			switch ( transmit( bundle ) )
//			{
//				case NO_ROUTE_FOUND:
//
//				break;
//
//				default:
//
//				break;
//			}
//		}

//		bool BundleCore::transmitBundle(const BundleSchedule &schedule, const Node &node)
//		{
//			// Bündel holen
//			const Bundle &bundle = schedule.getBundle();
//
//			try {
//				// Bündelparameter holen
//				PrimaryFlags flags = bundle.getPrimaryFlags();
//
//				try {
//					// transfer bundle to node
//					switch ( getConvergenceLayer()->transmit( bundle, node ) )
//					{
//						case TRANSMIT_SUCCESSFUL:
//							// transmittion successful
//							if ( flags.isCustodyRequested() )
//							{
//								// set custody timer
//								m_cm.setTimer( bundle, 5, 0 );
//							}
//
//							// report forwarded event
//							EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_FORWARDED) );
//						break;
//
//						case BUNDLE_ACCEPTED:
//							// should never happen!
//						break;
//
//						default:
//							// create a new schedule
//							EventSwitch::raiseEvent( new RouteEvent( bundle, ROUTE_FIND_SCHEDULE ) );
//						break;
//					};
//
//				} catch (NoNeighbourFoundException ex) {
//					// target node not available, reschedule the bundle
//					EventSwitch::raiseEvent( new RouteEvent( bundle, ROUTE_FIND_SCHEDULE ) );
//				}
//
//				return true;
//			} catch (Exception ex) {
//				// No space left in the storage.
//				// No route found.
//				// Doesn't fit or not allowed.
//				// No receiption!?
//				// Dismiss the bundle. I don't know what to do.
//				EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_DELETED) );
//			}
//
//			return false;
//		}

		void BundleCore::received(const ConvergenceLayer &cl, Bundle &b)
		{
			EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_RECEIVED) );
			EventSwitch::raiseEvent( new RouteEvent(b, ROUTE_PROCESS_BUNDLE) );
		}


//		void BundleCore::received(const ConvergenceLayer &cl, Bundle &b)
//		{
//			PrimaryFlags flags = b.getPrimaryFlags();
//
//			// received event
//			EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_RECEIVED) );
//
//			// check all block of the bundle for block flag actions
//			list<Block*> blocks = b.getBlocks();
//			list<Block*>::const_iterator iter = blocks.begin();
//
//			while (iter != blocks.end())
//			{
//				Block *block = (*iter);
//				BlockFlags flags = block->getBlockFlags();
//
//				if ( !block->isProcessed() )
//				{
//					if ( flags.getFlag(REPORT_IF_CANT_PROCESSED) )
//					{
//						// transmit a status report if requested
//						Bundle bundle = createStatusReport(b, RECEIPT_OF_BUNDLE, BLOCK_UNINTELLIGIBLE);
//						transmit( bundle );
//					}
//
//					if ( flags.getFlag(DELETE_IF_CANT_PROCESSED) )
//					{
//						// discard the hole bundle!
//						EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
//						return;
//					}
//
//					if ( flags.getFlag(DISCARD_IF_CANT_PROCESSED) )
//					{
//						// discard this block
//						b.removeBlock( block );
//					}
//				}
//
//				iter++;
//			}
//
//			// delete if the lifetime has expired
//			if (b.isExpired())
//			{
//				EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
//				return;
//			}
//
//			try {
//				// Wurde Custody angefordert?
//				PrimaryFlags flags( b.getInteger(PROCFLAGS) );
//
//				if (flags.isCustodyRequested())
//				{
//					// Setze Custody EID auf die eigene EID
//					b.setCustodian( m_localeid );
//				}
//
//				// find a route
//				EventSwitch::raiseEvent( new RouteEvent(b, ROUTE_FIND_SCHEDULE) );
//			} catch (InvalidBundleData ex) {
//
//			} catch (Exception ex) {
//				// Alle anderen Ausnahmen: NoSpaceLeftException, BundleExpiredException
//				// und NoScheduleFoundException führen zum ablehnen des Bündel.
//
//				// Bei Custody noch ein Custody NACK senden
//				if (flags.isCustodyRequested())
//				{
//					transmitCustody(false, b);
//				}
//
//				// Bündel verwerfen und ggf. Reports generieren
//				EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
//			}
//		}

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
