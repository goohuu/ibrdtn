#include "emma/MeasurementWorker.h"
#include "emma/MeasurementJob.h"
#include "emma/Measurement.h"
#include "core/EventSwitch.h"
#include "emma/PositionEvent.h"
#include "data/BundleFactory.h"
#include "data/PayloadBlockFactory.h"
#include "data/Bundle.h"

using namespace dtn::data;
using namespace dtn::core;

namespace emma
{
	MeasurementWorker::MeasurementWorker(BundleCore &core, MeasurementWorkerConfig config)
		: AbstractWorker(core, "/measurement"), Service("MeasurementWorker"), m_dtntime(0), m_datasize(0), m_config(config)
	{
		m_source = getWorkerURI();

		// register at event switch
		EventSwitch::registerEventReceiver(PositionEvent::className, this);
	}

	void MeasurementWorker::initialize()
	{
		EventSwitch::unregisterEventReceiver(PositionEvent::className, this);
	}

	void MeasurementWorker::terminate()
	{
	}

	MeasurementWorker::~MeasurementWorker()
	{
	}

	void MeasurementWorker::tick()
	{
		unsigned int curtime = BundleFactory::getDTNTime();

		if (m_dtntime <= curtime)
		{
			// Alle 5 Sekunden aktiv werden
			m_dtntime = curtime + m_config.interval;

			unsigned int jobcount = m_config.jobs.size();

//			if (m_gps != NULL)
//			{
//				jobcount++;
//			}

			// Vorherige Größe als Speichergröße verwenden
			Measurement m( m_datasize, jobcount );

//			// Wenn wir GPS haben das als erste Messung
//			if (m_gps != NULL)
//			{
//				m.add( m_gps );
//			}

			// Alle Jobs abarbeiten
			vector<MeasurementJob>::iterator iter = m_config.jobs.begin();

			while (iter != m_config.jobs.end())
			{
				// Job ausführen
				(*iter).execute();

				// Füge Daten des Jobs hinzu
				m.add( *iter );

				// Zum nächsten Job
				iter++;
			}

			BundleFactory &fac = BundleFactory::getInstance();

			// Erstelle ein Bundle mit den Messdaten als Payload
			Bundle *bundle = fac.newBundle();
			bundle->appendBlock( PayloadBlockFactory::newPayloadBlock(m.getData(), m.getLength()) );

			// Adressen und Lifetime setzen
			bundle->setDestination( m_config.destination );
			bundle->setSource( m_source );
			bundle->setInteger( LIFETIME, m_config.lifetime );

			// Custody erforderlich?
			if (m_config.custody)
			{
				PrimaryFlags flags = bundle->getPrimaryFlags();
				flags.setCustodyRequested(true);
				//flags.setFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE, true);
				//flags.setFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY, true);
				bundle->setPrimaryFlags( flags );
			}

			// Bündel versenden
			switch ( getCore().transmit( bundle ) )
			{
				case NO_ROUTE_FOUND:
					delete bundle;
				break;

				default:

				break;
			}

			// Speichergröße merken
			m_datasize = m.getLength();
		}

		usleep(50);
	}

	TransmitReport MeasurementWorker::callbackBundleReceived(const Bundle &b)
	{

	}

	void MeasurementWorker::raiseEvent(const Event *evt)
	{
		const PositionEvent *event = dynamic_cast<const PositionEvent*>(evt);

		if (event != NULL)
		{
			m_position = event->getPosition();
		}
	}
}
