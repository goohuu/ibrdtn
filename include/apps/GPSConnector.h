#ifndef GPSCONNECTOR_H_
#define GPSCONNECTOR_H_

#include "emma/GPSProvider.h"

#include <string>

using namespace std;

namespace emma
{
	class GPSConnector : public dtn::utils::JoinableThread, public GPSProvider
	{
		public:
			GPSConnector(string host, unsigned int port);
			virtual ~GPSConnector();

			virtual double getTime();
			virtual double getLongitude();
			virtual double getLatitude();

			virtual GPSState getState();

		protected:
			virtual void run();

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

			dtn::utils::Mutex m_datalock;
			dtn::utils::Mutex m_socketlock;

			unsigned int m_datatimeout;

			int m_socket;

			bool _running;
	};
}

#endif /*GPSCONNECTOR_H_*/
