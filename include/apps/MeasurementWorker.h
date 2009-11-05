#ifndef MEASUREMENTWORKER_H_
#define MEASUREMENTWORKER_H_

#include "emma/MeasurementJob.h"
#include "emma/GPSProvider.h"
#include "core/AbstractWorker.h"
#include "core/EventReceiver.h"
#include "ibrdtn/data/EID.h"

using namespace std;
using namespace dtn::core;

namespace emma
{
	struct MeasurementWorkerConfig
	{
		unsigned int interval;
		string destination;
		unsigned int lifetime;
		bool custody;
		vector<MeasurementJob> jobs;
	};

	class MeasurementWorker : public AbstractWorker, public dtn::utils::JoinableThread, public EventReceiver
	{
	public:
		MeasurementWorker(MeasurementWorkerConfig config);
		virtual ~MeasurementWorker();

		unsigned char* needMore(unsigned char* data, unsigned int used, unsigned int needed);

		/**
		 * method to receive PositionEvent from EventSwitch
		 */
		void raiseEvent(const dtn::core::Event *evt);

		TransmitReport callbackBundleReceived(const Bundle &b);

	protected:
		void run();

	private:
		unsigned int m_dtntime;
		unsigned int m_datasize;
		EID m_source;

		MeasurementWorkerConfig m_config;
		pair<double,double> m_position;
		bool _running;
	};
}

#endif /*MEASUREMENTWORKER_H_*/
