#ifndef MEASUREMENTWORKER_H_
#define MEASUREMENTWORKER_H_

#include "emma/MeasurementJob.h"
#include "emma/GPSProvider.h"
#include "core/AbstractWorker.h"
#include "core/EventReceiver.h"
#include "utils/Service.h"
#include <string>

using namespace std;
using namespace dtn::core;
using namespace dtn::utils;

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

	class MeasurementWorker : public AbstractWorker, public Service, public EventReceiver
	{
	public:
		MeasurementWorker(MeasurementWorkerConfig config);
		~MeasurementWorker();
		void tick();
		unsigned char* needMore(unsigned char* data, unsigned int used, unsigned int needed);

		/**
		 * method to receive PositionEvent from EventSwitch
		 */
		void raiseEvent(const Event *evt);

		TransmitReport callbackBundleReceived(const Bundle &b);

	protected:
		virtual void initialize();
		virtual void terminate();

	private:
		unsigned int m_dtntime;
		unsigned int m_datasize;
		string m_source;

		MeasurementWorkerConfig m_config;
		pair<double,double> m_position;
	};
}

#endif /*MEASUREMENTWORKER_H_*/
