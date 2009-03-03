#ifndef MEASUREMENTWORKER_H_
#define MEASUREMENTWORKER_H_

#include "emma/MeasurementJob.h"
#include "emma/GPSProvider.h"
#include "core/AbstractWorker.h"
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
		vector<MeasurementJob*> jobs;
	};

	class MeasurementWorker : public AbstractWorker, public Service
	{
	public:
		MeasurementWorker(BundleCore *core, MeasurementWorkerConfig config);
		~MeasurementWorker();
		void tick();
		unsigned char* needMore(unsigned char* data, unsigned int used, unsigned int needed);
		void setGPSProvider(GPSProvider *gpsconn);

	protected:
		virtual void initialize();
		virtual void terminate();

	private:
		unsigned int m_dtntime;
		unsigned int m_datasize;
		string m_source;

		GPSProvider *m_gps;
		MeasurementWorkerConfig m_config;
	};
}

#endif /*MEASUREMENTWORKER_H_*/
