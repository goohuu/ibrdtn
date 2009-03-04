#include "core/BundleCore.h"
#include "core/BundleSchedule.h"
#include "core/AbstractWorker.h"
#include "core/CustodyManager.h"

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
		 : Service("BundleCore"), m_clayer(NULL), m_storage(NULL), m_router(NULL), m_custodymanager(NULL), m_bundlewaiting(false), m_localeid(localeid)
		{
		}

		BundleCore::~BundleCore()
		{
		}

		void BundleCore::setStorage(BundleStorage *bs)
		{
			m_storage = bs;
		}

		BundleStorage* BundleCore::getStorage()
		{
			return m_storage;
		}

		void BundleCore::setCustodyManager(CustodyManager *custodymanager)
		{
			m_custodymanager = custodymanager;
		}

		CustodyManager* BundleCore::getCustodyManager()
		{
			return m_custodymanager;
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

		void BundleCore::setBundleRouter(BundleRouter *router)
		{
			m_router = router;
		}

		BundleRouter* BundleCore::getBundleRouter()
		{
			return m_router;
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
				// Frage den Router nach den nächsten Empfänger und eine Sendezeit
				BundleSchedule schedule = getBundleRouter()->getSchedule(b);

				// Wurde Custody angefordert?
				PrimaryFlags flags( b->getInteger(PROCFLAGS) );

				if (flags.isCustodyRequested())
				{
					// Setze Custody EID auf die eigene EID
					b->setCustodian( m_localeid );
				}

				// Speichere Bundle im persistenten Speicher
				getStorage()->store(schedule);

				// Signalisiere, dass ein Bündel wartet
				m_bundlewaiting = true;

				return BUNDLE_ACCEPTED;
			} catch (NoSpaceLeftException ex) {
				// Bündel verwerfen
				dismissBundle( b, ex.what() );
				return UNKNOWN;
			} catch (BundleExpiredException ex) {
				return UNKNOWN;
			} catch (NoScheduleFoundException ex) {
				return NO_ROUTE_FOUND;
			};
		}

		Bundle* BundleCore::createStatusReport(Bundle *b, StatusReportType type, StatusReportReasonCode reason)
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
			bundle->setDestination(b->getReportTo());

			// set bundle parameter
			report->setMatch(b);

			return bundle;
		}

		Bundle* BundleCore::createCustodySignal(Bundle *b, bool accepted)
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
			bundle->setDestination(b->getCustodian());

			// set bundle parameter
			custody->setMatch(b);

			return bundle;
		}

		void BundleCore::transmitCustody(bool accept, Bundle *b)
		{
			PrimaryFlags flags = b->getPrimaryFlags();
			Bundle *bundle;

			// Senden an
			string eid = b->getCustodian();

			// Custody-Acceptance Report senden?
			if ( flags.getFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE) )
			{
				transmit( createStatusReport(b, CUSTODY_ACCEPTANCE_OF_BUNDLE) );
			}

			// Erstelle ein CustodySignal
			bundle = createCustodySignal(b, accept);

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

		bool BundleCore::transmitBundle(BundleSchedule schedule)
		{
			// Variable für das Bündel initialisieren
			Bundle *bundle = NULL;

			try {
				// Bündel holen
				bundle = schedule.getBundle();

				// Bündelparameter holen
				PrimaryFlags flags = bundle->getPrimaryFlags();

				if ( flags.isCustodyRequested() && !m_custodymanager->timerAvailable() )
				{
					// Zurück in die Bundle Storage legen für eine Sekunde später
					getStorage()->store( BundleSchedule( bundle, schedule.getTime() + 1, schedule.getEID() ) );
					return true;
				}

				try {
					// Knoten an den gesendet werden soll
					Node node = getBundleRouter()->getNeighbour( schedule.getEID() );

					// transfer bundle to node
					switch ( getConvergenceLayer()->transmit( bundle, node ) )
					{
						case TRANSMIT_SUCCESSFUL:
							// transmittion successful
							if ( flags.isCustodyRequested() )
							{
								// create a timer, if custody is requested
								m_custodymanager->setTimer( node, bundle, 1, 1 );
							}
							else
							{
								// report forwarded
								if ( flags.getFlag(REQUEST_REPORT_OF_BUNDLE_FORWARDING) )
								{
									transmit( createStatusReport(bundle, FORWARDING_OF_BUNDLE) );
								}

								// delete the bundle, if no custody is requested
								delete bundle;
							}
						break;

						case BUNDLE_ACCEPTED:
							// should never happend!
						break;

						default:
							// create a new schedule
							getStorage()->store( getBundleRouter()->getSchedule( bundle ) );
						break;
					};

				} catch (NoNeighbourFoundException ex) {
					// target node not available, reschedule the bundle
					getStorage()->store( getBundleRouter()->getSchedule( bundle ) );
				}

				return true;
			} catch (TransferNotCompletedException ex) {
				Bundle *fragment = NULL;
				try {
					// Get the fragment of the remaining data.
					fragment = ex.getFragment();

					if (fragment != NULL)
					{
						// Store the fragment in the storage.
						getStorage()->store( BundleSchedule( fragment, schedule.getTime(), schedule.getEID() ) );
					}

					// We have fragments, delete the origin bundle.
					delete bundle;

				} catch (DoNotFragmentBitSetException ex) {
					// This bundle can't be fragmented. Dismiss it!
					dismissBundle( fragment, ex.what() );
				} catch (NoSpaceLeftException ex) {
					// No space left in the storage. Dismiss the bundle!
					dismissBundle( fragment, ex.what() );
				}
			} catch (Exception ex) {
				// No space left in the storage.
				// No route found.
				// Doesn't fit or not allowed.
				// No receiption!?
				// Dismiss the bundle. I don't know what to do.
				if (bundle != NULL) dismissBundle( bundle, ex.what() );
			}

			return false;
		}

		void BundleCore::received(ConvergenceLayer *cl, Bundle *b)
		{
			PrimaryFlags flags = b->getPrimaryFlags();

			// Report received
			if ( b->getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_RECEPTION) )
			{
				transmit( createStatusReport(b, RECEIPT_OF_BUNDLE) );
			}

			// check all block of the bundle for block flag actions
			list<Block*> blocks = b->getBlocks();
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
						dismissBundle(b);
						return;
					}

					if ( flags.getFlag(DISCARD_IF_CANT_PROCESSED) )
					{
						// discard this block
						b->removeBlock( block );
					}
				}

				iter++;
			}

			try {
				// Ist das Bundle lokal?
				if ( getBundleRouter()->isLocal( b ) )
				{
					// Wenn Custody angefordert ist
					if (flags.isCustodyRequested())
					{
						// Sende ein Custody ACK
						transmitCustody(true, b);

						// Setze eigene EID als Custodian
						b->setCustodian( m_localeid );
					}

					// Prüfe auf Custody-Signale und StatusReports
					if ( flags.isAdmRecord() )
					{
						processCustody( b );
						processStatusReport( b );
					}

					// Bundle ist lokal
					forwardLocalBundle( b );
				}
				else
				{
					// Frage den Router nach dem nächsten Empfänger und eine Sendezeit
					BundleSchedule schedule = getBundleRouter()->getSchedule(b);

					// Wenn Custody angefordert ist
					if (flags.isCustodyRequested())
					{
						// Sende ein Custody ACK
						transmitCustody(true, b);

						// Setze eigene EID als Custodian
						b->setCustodian( m_localeid );
					}

					// Lege es den Schedule in der Storage ab
					getStorage()->store(schedule);
				}
			} catch (InvalidBundleData ex) {
				delete b;
			} catch (Exception ex) {
				// Alle anderen Ausnahmen: NoSpaceLeftException, BundleExpiredException
				// und NoScheduleFoundException führen zum ablehnen des Bündel.

				// Bei Custody noch ein Custody NACK senden
				if (flags.isCustodyRequested())
				{
					transmitCustody(false, b);
				}

				// Bündel verwerfen und ggf. Reports generieren
				dismissBundle(b, ex.what() );
			}
		}

		void BundleCore::dismissBundle(Bundle *b, string reason)
		{
			// Das Bundle wird verworfen. Das heißt es müssten eventuell passende
			// StatusReports generiert werden.
			if ( b->getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_DELETION) )
			{
				transmit( createStatusReport(b, DELETION_OF_BUNDLE) );
			}
			delete b;
		}

		void BundleCore::tick()
		{
			// Aktuelle DTN Zeit holen
			unsigned int dtntime = BundleFactory::getDTNTime();

			// Setze das Bündelsignal auf false
			m_bundlewaiting = false;

			// Anzahl der Bündel in der Storage holen
			unsigned int bcount = getStorage()->getCount();

			list<Node> nodes = getBundleRouter()->getNeighbours();
			list<Node>::iterator iter = nodes.begin();

			while (iter != nodes.end())
			{
				try
				{
					while (bcount > 0)
					{
						transmitBundle( getStorage()->getSchedule( (*iter).getURI() ) );
						bcount--;
					}
				} catch (exceptions::NoScheduleFoundException ex) {

				}

				iter++;
			}

			try {
				while (bcount > 0)
				{
					// Hole ein Schedule aus der Storage von einem Bundle das jetzt versendet werden sollen
					transmitBundle( getStorage()->getSchedule(dtntime) );
					bcount--;
				}
			} catch (exceptions::NoScheduleFoundException ex) {

			}

			// Warte bis sich die DTNTime ändert oder eine Nachricht versendet wurde

			while (true)
			{
				if ( (dtntime != BundleFactory::getDTNTime()) || m_bundlewaiting )
				{
					return;
				}
				usleep(1000);
			}
		}

		void BundleCore::forwardLocalBundle(Bundle *bundle)
		{
			AbstractWorker *worker = NULL;

			Bundle *deliveryReport = NULL;
			Bundle *deletionReport = NULL;

			PrimaryFlags flags;

			if (bundle != NULL)
			{
				// Flags speichern
				flags = bundle->getPrimaryFlags();

				// Prüfen ob das Bundle ein Fragment ist
				if ( flags.isFragment() )
				{
					// Einen Report für dieses Bundle versenden
					deliveryReport = createStatusReport(bundle, DELIVERY_OF_BUNDLE);

					// Bundle an die Storage zum reassemblieren übergeben
					bundle = getStorage()->storeFragment(bundle);

					// Report Bundle delivered
					if ( flags.getFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY) )
					{
						transmit(deliveryReport);
					}
					else
					{
						delete deliveryReport;
					}

					deliveryReport = NULL;
				};
			}

			if (bundle != NULL)
			{
				// Flags speichern
				flags = bundle->getPrimaryFlags();

				deliveryReport = createStatusReport(bundle, DELIVERY_OF_BUNDLE);
				deletionReport = createStatusReport(bundle, DELETION_OF_BUNDLE);

				MutexLock l(m_workerlock);

				worker = m_worker[ bundle->getDestination() ];
				if (worker != NULL)
				{
					switch ( worker->callbackBundleReceived( bundle ) )
					{
						case BUNDLE_ACCEPTED:
							// Report Bundle delivered
							if ( flags.getFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY) )
							{
								transmit(deliveryReport);
								deliveryReport = NULL;
							}
						break;

						default:
							// Report deleted
							if ( flags.getFlag(REQUEST_REPORT_OF_BUNDLE_DELETION) )
							{
								transmit(deletionReport);
								deletionReport = NULL;
							}
							delete bundle;
						break;
					}
				}
				else
				{
					// Report deleted
					if ( flags.getFlag(REQUEST_REPORT_OF_BUNDLE_DELETION) )
					{
						transmit(deletionReport);
						deletionReport = NULL;
					}
					delete bundle;
				}
			}

			if (deletionReport != NULL) delete deletionReport;
			if (deliveryReport != NULL) delete deliveryReport;
		}

		void BundleCore::processStatusReport(Bundle *b)
		{
			// Suche im Bundle nach StatusReport-Blöcken
			Block *block = b->getPayloadBlock();
			StatusReportBlock *report = NULL;

			if (block != NULL)
			{
				report = dynamic_cast<StatusReportBlock*>( block );
			}
		}

		void BundleCore::processCustody(Bundle *b)
		{
			// search for custody blocks
			Block *block = b->getPayloadBlock();
			CustodySignalBlock *signal = NULL;

			if (block != NULL)
			{
				signal = dynamic_cast<CustodySignalBlock*>( block );

				if ( (signal != NULL) && (signal->isAccepted()) )
				{
					// remove the timer for this bundle
					Bundle *bundle = m_custodymanager->removeTimer( b->getSource(), signal );

					if (bundle != NULL)
					{
						// report forwarded
						if ( bundle->getPrimaryFlags().getFlag(REQUEST_REPORT_OF_BUNDLE_FORWARDING) )
						{
							transmit( createStatusReport(bundle, FORWARDING_OF_BUNDLE) );
						}

						delete bundle;
					}
				}
				else
				{
					// custody was rejected
					// TODO: replaning the bundle route
				}
			}
		}

		void BundleCore::registerSubNode(string eid, AbstractWorker *node)
		{
			MutexLock l(m_workerlock);
			m_worker[m_localeid + eid] = node;
		}

		void BundleCore::unregisterSubNode(string eid)
		{
			m_worker.erase(m_localeid + eid);
		}

		void BundleCore::triggerCustodyTimeout(CustodyTimer timer)
		{
			if ( timer.getAttempt() >= 10 )
			{
				// Bundle nicht zustellbar, wir planen neu
				try {
					// Erstelle ein neuen Schedule
					getStorage()->store( getBundleRouter()->getSchedule( timer.getBundle() ) );
				} catch (Exception ex) {
					// Kein Platz zum aufbewahren oder keine andere Route gefunden
					dismissBundle( timer.getBundle(), ex.what() );
				}
			}
			else
			{
				// Übertragungswiederholung durchführen
				switch ( getConvergenceLayer()->transmit( timer.getBundle(), timer.getCustodyNode() ) )
				{
					case TRANSMIT_SUCCESSFUL:
						// Übermittlung erfolgreich, erstelle einen Timer
						m_custodymanager->setTimer( timer.getCustodyNode(), timer.getBundle(), 1, timer.getAttempt() + 1 );
					break;

					case BUNDLE_ACCEPTED:
						// Sollte nie auftreten!
					break;

					default:
						// Bundle konnte nicht übertragen werden.
					break;
				};
			}
		}

		string BundleCore::getLocalURI() const
		{
			return m_localeid;
		}
	}
}
