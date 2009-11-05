#include "emma/MeasurementWorker.h"
#include "emma/MeasurementJob.h"
#include "emma/Measurement.h"
#include "core/EventSwitch.h"
#include "emma/PositionEvent.h"

#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/utils/Utils.h"

using namespace dtn::data;
using namespace dtn::core;

namespace emma
{
	MeasurementWorker::MeasurementWorker(MeasurementWorkerConfig config)
		: AbstractWorker("/measurement"), m_dtntime(0), m_datasize(0), m_config(config), _running(true)
	{
		m_source = getWorkerURI();

		// register at event switch
		EventSwitch::registerEventReceiver(PositionEvent::className, this);
	}

	MeasurementWorker::~MeasurementWorker()
	{
		_running = false;
		join();
	}

	void MeasurementWorker::run()
	{
		while (_running)
		{
			unsigned int curtime = dtn::utils::Utils::get_current_dtn_time();

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

				// create a bundle with the measurement data as payload
				Bundle bundle;
				PayloadBlock *block = new PayloadBlock();

				// append the data
				block->getBLOBReference().append((char*)m.getData(), m.getLength());

				// set adresses and lifetime
				bundle._destination = m_config.destination;
				bundle._source = m_source;
				bundle._lifetime = m_config.lifetime;

				// Custody required?
				if (m_config.custody)
				{
					if (!(bundle._procflags & Bundle::CUSTODY_REQUESTED)) bundle._procflags += Bundle::CUSTODY_REQUESTED;
				}

				// commit and append the block
				bundle.addBlock(block);

				// transmit the bundle
				transmit( bundle );

				// remind the size of the measurement
				m_datasize = m.getLength();
			}

			this->sleep(500);
			yield();
		}
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
