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
	MeasurementWorker::MeasurementWorker(MeasurementWorkerConfig config)
		: AbstractWorker("/measurement"), Service("MeasurementWorker"), m_dtntime(0), m_datasize(0), m_config(config)
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
			// run every 5 seconds
			m_dtntime = curtime + m_config.interval;

			unsigned int jobcount = m_config.jobs.size();

			// use previous size
			Measurement m( m_datasize, jobcount );

			// Add GPS position to the measurement
			jobcount++;
			m.add( m_position );

			// walk through all jobs
			vector<MeasurementJob>::iterator iter = m_config.jobs.begin();

			while (iter != m_config.jobs.end())
			{
				// execute job
				(*iter).execute();

				// add data of the jobs
				m.add( *iter );

				// next job
				iter++;
			}

			BundleFactory &fac = BundleFactory::getInstance();

			// create a bundle with the measurement data as payload
			Bundle *bundle = fac.newBundle();
			bundle->appendBlock( PayloadBlockFactory::newPayloadBlock(m.getData(), m.getLength()) );

			// set adresses and lifetime
			bundle->setDestination( m_config.destination );
			bundle->setSource( m_source );
			bundle->setInteger( LIFETIME, m_config.lifetime );

			// Custody required?
			if (m_config.custody)
			{
				PrimaryFlags flags = bundle->getPrimaryFlags();
				flags.setCustodyRequested(true);
				//flags.setFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE, true);
				//flags.setFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY, true);
				bundle->setPrimaryFlags( flags );
			}

			// transmit the bundle
			transmit( *bundle );
			delete bundle;

			// remind the size of the measurement
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
