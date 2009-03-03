#ifndef GPSCONNECTOR_H_
#define GPSCONNECTOR_H_

#include "emma/GPSProvider.h"
#include "utils/Service.h"
#include "utils/MutexLock.h"
#include "utils/Mutex.h"
#include <string>

using namespace dtn::utils;
using namespace std;

namespace emma
{
	class GPSConnector : public Service, public GPSProvider
	{
		public:
			GPSConnector(string host, unsigned int port);
			virtual ~GPSConnector();

			virtual double getTime();
			virtual double getLongitude();
			virtual double getLatitude();

			virtual GPSState getState();

		protected:
			virtual void tick();
			virtual void initialize();
			virtual void terminate();

		private:
			bool requestData();
			bool readData();
			void parseData(char *data, unsigned int size);
			bool connect();
			void disconnect();

			void setState(GPSState state);

			string m_host;
			unsigned int m_port;

			GPSState m_state;
			double m_longitude;
			double m_latitude;
			double m_time;

			Mutex m_datalock;
			Mutex m_socketlock;

			unsigned int m_datatimeout;

			int m_socket;
	};
}

#endif /*GPSCONNECTOR_H_*/
