#ifndef MEASUREMENT_H_
#define MEASUREMENT_H_

#include "emma/MeasurementJob.h"
#include "emma/GPSProvider.h"

namespace emma
{
	class Measurement
	{
	public:
		Measurement(unsigned int datasize, unsigned int jobs);
		~Measurement();

		void add(unsigned char type, char* job_data, unsigned int job_length);
		void add(MeasurementJob &job);
		void add(GPSProvider &gps);
		void add(unsigned char type, double value);

		unsigned char* getData();
		unsigned int getLength();

	private:
		void need(unsigned int needed);

		unsigned int m_datasize;
		unsigned char* m_data;
		unsigned int m_length;
	};
}

#endif /*MEASUREMENT_H_*/
