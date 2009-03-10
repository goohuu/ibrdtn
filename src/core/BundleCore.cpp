#include "core/BundleCore.h"
#include "core/BundleSchedule.h"
#include "core/AbstractWorker.h"
#include "core/CustodyManager.h"
#include "core/EventSwitch.h"
#include "core/StorageEvent.h"
#include "core/RouteEvent.h"
#include "core/CustodyEvent.h"
#include "core/BundleEvent.h"

#include "data/Bundle.h"
#include "data/Exceptions.h"
#include "data/AdministrativeBlock.h"
#include "data/PrimaryFlags.h"
#include "data/PayloadBlockFactory.h"

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
		BundleCore::BundleCore(string localeid)
		 : m_clayer(NULL), m_localeid(localeid)
		{
			// register me for events
			EventSwitch::registerEventReceiver( RouteEvent::className, this );
			EventSwitch::registerEventReceiver( CustodyEvent::className, this );
			EventSwitch::registerEventReceiver( BundleEvent::className, this );
		}

		BundleCore::~BundleCore()
		{
			EventSwitch::unregisterEventReceiver( RouteEvent::className, this );
			EventSwitch::unregisterEventReceiver( CustodyEvent::className, this );
			EventSwitch::unregisterEventReceiver( BundleEvent::className, this );
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

		void BundleCore::raiseEvent(const Event *evt)
		{
			const RouteEvent *routeevent = dynamic_cast<const RouteEvent*>(evt);
			const CustodyEvent *custodyevent = dynamic_cast<const CustodyEvent*>(evt);
			const BundleEvent *bundleevent = dynamic_cast<const BundleEvent*>(evt);

			if (bundleevent != NULL)
			{
				const Bundle &b = bundleevent->getBundle();

				switch (bundleevent->getAction())
				{
				case BUNDLE_RECEIVED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_RECEPTION) )
					{
						transmit( createStatusReport(b, RECEIPT_OF_BUNDLE) );
					}
					break;
				case BUNDLE_DELETED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_DELETION) )
					{
						transmit( createStatusReport(b, DELETION_OF_BUNDLE) );
					}
					break;

				case BUNDLE_FORWARDED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_FORWARDING) )
					{
						transmit( createStatusReport(b, FORWARDING_OF_BUNDLE) );
					}
					break;

				case BUNDLE_DELIVERED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY) )
					{
						transmit( createStatusReport(b, DELIVERY_OF_BUNDLE) );
					}
					break;

				case BUNDLE_CUSTODY_ACCEPTED:
					if ( b.getPrimaryFlags().getFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE) )
					{
						transmit( createStatusReport(b, CUSTODY_ACCEPTANCE_OF_BUNDLE) );
					}
					break;
				}
			}
			else if (custodyevent != NULL)
			{
				switch (custodyevent->getAction())
				{
					case CUSTODY_ACCEPTANCE:
					{
						// send a custody ack
						transmitCustody(true, custodyevent->getBundle());

						// set us as custodian
						//b->setCustodian( m_localeid );

						break;
					}

					case CUSTODY_TIMEOUT:
					{
//						if ( timer.getAttempt() >= 10 )
//						{
//							// Bundle nicht zustellbar, wir planen neu
//							try {
//								// Erstelle ein neuen Schedule
//								EventSwitch::raiseEvent( new RouteEvent( timer.getBundle(), ROUTE_FIND_SCHEDULE ) );
//							} catch (Exception ex) {
//								// Kein Platz zum aufbewahren oder keine andere Route gefunden
//								dismissBundle( timer.getBundle(), ex.what() );
//							}
//						}
//						else
//						{
//							// Übertragungswiederholung durchführen
//							switch ( getConvergenceLayer()->transmit( timer.getBundle(), timer.getCustodyNode() ) )
//							{
//								case TRANSMIT_SUCCESSFUL:
//									// Übermittlung erfolgreich, erstelle einen Timer
//									m_custodymanager->setTimer( timer.getCustodyNode(), timer.getBundle(), 1, timer.getAttempt() + 1 );
//								break;
//
//								case BUNDLE_ACCEPTED:
//									// Sollte nie auftreten!
//								break;
//
//								default:
//									// Bundle konnte nicht übertragen werden.
//								break;
//							};
//						}
						break;
					}
				}
			}
			else if (routeevent != NULL)
			{
				switch (routeevent->getAction())
				{
					case ROUTE_FIND_SCHEDULE:
						break;
					case ROUTE_LOCAL_BUNDLE:
					{
						// get the bundle of the event
						const Bundle &b = routeevent->getBundle();
						PrimaryFlags flags = b.getPrimaryFlags();

						// check for administrative records
						if ( flags.isAdmRecord() )
						{
							processCustody( b );
						}

						// forward the bundle to the application
						forwardLocalBundle( b );
						break;
					}
					case ROUTE_TRANSMIT_BUNDLE:
					{
						transmitBundle(routeevent->getSchedule(), routeevent->getNode());
						break;
					}
				}

			}
		}

		TransmitReport BundleCore::transmit(Bundle *b)
		{
			// check all block of the bundle for block flag actions
			list<Block*> blocks = b->getBlocks();
			list<Block*>::const_iterator iter = blocks.begin();

			while (iter != blocks.end())
			{
				Block *block = (*iter);
				BlockFlags flags = block->getBlockFlags();

				if ( !block->isProcessed() )
				{
					// if forwarded without processed, mark it!
					flags.setFlag(FORWARDED_WITHOUT_PROCESSED, true);
					block->setBlockFlags(flags);
					block->updateBlockSize();
				}

				iter++;
			}

			try {
				// Wurde Custody angefordert?
				PrimaryFlags flags( b->getInteger(PROCFLAGS) );

				if (flags.isCustodyRequested())
				{
					// Setze Custody EID auf die eigene EID
					b->setCustodian( m_localeid );
				}

				// find a next route for the bundle
				EventSwitch::raiseEvent( new RouteEvent(*b, ROUTE_FIND_SCHEDULE) );

				delete b;
				return BUNDLE_ACCEPTED;
			} catch (NoSpaceLeftException ex) {
				// Bündel verwerfen
				EventSwitch::raiseEvent( new BundleEvent(*b, BUNDLE_DELETED) );
				delete b;
				return UNKNOWN;
			} catch (BundleExpiredException ex) {
				return UNKNOWN;
			} catch (NoScheduleFoundException ex) {
				return NO_ROUTE_FOUND;
			};
		}

		Bundle* BundleCore::createStatusReport(const Bundle &b, StatusReportType type, StatusReportReasonCode reason)
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

			return bundle;
		}

		Bundle* BundleCore::createCustodySignal(const Bundle &b, bool accepted)
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

			return bundle;
		}

		void BundleCore::transmitCustody(bool accept, const Bundle &b)
		{
			PrimaryFlags flags = b.getPrimaryFlags();

			// Senden an
			string eid = b.getCustodian();

			// Custody-Acceptance Event
			EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_CUSTODY_ACCEPTED) );

			// Erstelle ein CustodySignal
			Bundle *bundle = createCustodySignal(b, accept);

			// Absender setzen
			bundle->setSource( m_localeid );

			switch ( transmit( bundle ) )
			{
				case NO_ROUTE_FOUND:

				break;

				default:

				break;
			}
		}

		bool BundleCore::transmitBundle(const BundleSchedule &schedule, const Node &node)
		{
			// Bündel holen
			const Bundle &bundle = schedule.getBundle();

			try {
				// Bündelparameter holen
				PrimaryFlags flags = bundle.getPrimaryFlags();

				try {
					// transfer bundle to node
					switch ( getConvergenceLayer()->transmit( bundle, node ) )
					{
						case TRANSMIT_SUCCESSFUL:
							// transmittion successful
							if ( flags.isCustodyRequested() )
							{
								// set custody timer
								EventSwitch::raiseEvent( new CustodyEvent( bundle, CUSTODY_ACCEPTANCE ) );
							}

							// report forwarded event
							EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_FORWARDED) );
						break;

						case BUNDLE_ACCEPTED:
							// should never happen!
						break;

						default:
							// create a new schedule
							EventSwitch::raiseEvent( new RouteEvent( bundle, ROUTE_FIND_SCHEDULE ) );
						break;
					};

				} catch (NoNeighbourFoundException ex) {
					// target node not available, reschedule the bundle
					EventSwitch::raiseEvent( new RouteEvent( bundle, ROUTE_FIND_SCHEDULE ) );
				}

				return true;
			} catch (Exception ex) {
				// No space left in the storage.
				// No route found.
				// Doesn't fit or not allowed.
				// No receiption!?
				// Dismiss the bundle. I don't know what to do.
				EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_DELETED) );
			}

			return false;
		}

		void BundleCore::received(const ConvergenceLayer &cl, Bundle &b)
		{
			PrimaryFlags flags = b.getPrimaryFlags();

			// received event
			EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_RECEIVED) );

			// check all block of the bundle for block flag actions
			list<Block*> blocks = b.getBlocks();
			list<Block*>::const_iterator iter = blocks.begin();

			while (iter != blocks.end())
			{
				Block *block = (*iter);
				BlockFlags flags = block->getBlockFlags();

				if ( !block->isProcessed() )
				{
					if ( flags.getFlag(REPORT_IF_CANT_PROCESSED) )
					{
						// transmit a status report if requested
						transmit( createStatusReport(b, RECEIPT_OF_BUNDLE, BLOCK_UNINTELLIGIBLE) );
					}

					if ( flags.getFlag(DELETE_IF_CANT_PROCESSED) )
					{
						// discard the hole bundle!
						EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
						return;
					}

					if ( flags.getFlag(DISCARD_IF_CANT_PROCESSED) )
					{
						// discard this block
						b.removeBlock( block );
					}
				}

				iter++;
			}

			// delete if the lifetime has expired
			if (b.isExpired())
			{
				EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
				return;
			}

			try {
				// Wurde Custody angefordert?
				PrimaryFlags flags( b.getInteger(PROCFLAGS) );

				if (flags.isCustodyRequested())
				{
					// Setze Custody EID auf die eigene EID
					b.setCustodian( m_localeid );
				}

				// find a route
				EventSwitch::raiseEvent( new RouteEvent(b, ROUTE_FIND_SCHEDULE) );
			} catch (InvalidBundleData ex) {

			} catch (Exception ex) {
				// Alle anderen Ausnahmen: NoSpaceLeftException, BundleExpiredException
				// und NoScheduleFoundException führen zum ablehnen des Bündel.

				// Bei Custody noch ein Custody NACK senden
				if (flags.isCustodyRequested())
				{
					transmitCustody(false, b);
				}

				// Bündel verwerfen und ggf. Reports generieren
				EventSwitch::raiseEvent( new BundleEvent(b, BUNDLE_DELETED) );
			}
		}

		void BundleCore::forwardLocalBundle(const Bundle &bundle)
		{
			// Flags speichern
			PrimaryFlags flags = bundle.getPrimaryFlags();

			// Prüfen ob das Bundle ein Fragment ist
			if ( flags.isFragment() )
			{
				// Bundle an die Storage zum reassemblieren übergeben
				EventSwitch::raiseEvent( new StorageEvent( bundle ) );

				return;
			};

			AbstractWorker *worker = m_worker[ bundle.getDestination() ];
			if (worker != NULL)
			{
				switch ( worker->callbackBundleReceived( bundle ) )
				{
					case BUNDLE_ACCEPTED:
						// raise delivered event
						EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_DELIVERED) );
					break;

					default:
						// raise deleted event
						EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_DELETED) );
					break;
				}
			}
			else
			{
				// raise deleted event
				EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_DELETED) );
			}
		}

		void BundleCore::processCustody(const Bundle &b)
		{
			// search for custody blocks
			Block *block = b.getPayloadBlock();
			CustodySignalBlock *signal = NULL;

			if (block != NULL)
			{
				signal = dynamic_cast<CustodySignalBlock*>( block );

				if ( signal != NULL )
				{
					// remove the timer for this bundle
					EventSwitch::raiseEvent( new CustodyEvent( b.getSource(), signal ) );
				}
			}
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
