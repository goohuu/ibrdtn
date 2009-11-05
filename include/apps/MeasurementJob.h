#ifndef MEASUREMENTJOB_H_
#define MEASUREMENTJOB_H_

#include <string>

using namespace std;

namespace emma
{
	class MeasurementJob
	{
		public:
		MeasurementJob(unsigned char type, string cmd);
		virtual ~MeasurementJob();

		void execute();
		unsigned char* getData();
		unsigned int getLength();
		unsigned char getType();

		private:
		string m_command;
		unsigned char m_type;
		unsigned char* m_data;
		unsigned int m_size;
	};
}

#endif /*MEASUREMENTJOB_H_*/
