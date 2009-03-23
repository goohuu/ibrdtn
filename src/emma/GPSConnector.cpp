#include "emma/GPSConnector.h"
#include "utils/Utils.h"
#include <iostream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <poll.h>
#include <iomanip>
#include <cstdlib>
#include <cstring>

using namespace std;
using namespace dtn::utils;

namespace emma
{
	GPSConnector::GPSConnector(string host, unsigned int port)
		: Service("GPSConnector"), m_host(host), m_port(port), m_state(DISCONNECTED), m_longitude(0), m_latitude(0), m_time(0), m_datatimeout(0), m_socket(0)
	{
	}

	GPSConnector::~GPSConnector()
	{
	}

	bool GPSConnector::connect()
	{
		MutexLock l1(m_socketlock);

		struct sockaddr_in address;

		address.sin_family = AF_INET;
		address.sin_addr.s_addr = inet_addr(m_host.c_str());
		address.sin_port = htons(m_port);

		// Create socket for client connection.
		m_socket = socket(AF_INET, SOCK_STREAM, 0);

		if (m_socket < 0)
		{
			return false;
		}

		if (::connect(m_socket, (sockaddr*)&address, sizeof(address)) < 0)
		{
			return false;
		}

		// Daten anfordern
		char data[] = { "w\r\n" };
		if (send(m_socket, data, strlen(data), 0) < 0)
		{
			close(m_socket);
			return false;
		}

		return true;
	}

	void GPSConnector::disconnect()
	{
		MutexLock l(m_socketlock);
		if ( getState() > DISCONNECTED )
		{
			close(m_socket);
		}
	}

	bool GPSConnector::requestData()
	{
		string request = "o\n";
		if (send(m_socket, request.c_str(), request.length() + 1, 0) < 0)
		{
			return false;
		}

		return false;
	}

	bool GPSConnector::readData()
	{
		// First look for data, wait max. 100ms
		struct pollfd psock[1];

		psock[0].fd = m_socket;
		psock[0].events = POLLIN;
		psock[0].revents = POLLERR;

		switch (poll(psock, 1, 10))
		{
			case 0:
				// Timeout

			break;

			case -1:
				// Error
				disconnect();
				setState( DISCONNECTED );
			break;

			default:
				char data[1024];

				// Data waiting
				int len = recv(m_socket, data, 1024, MSG_PEEK);

				if (len < 0)
				{
					// Keine Daten da.
					return false;
				}

				// look for a whole line in the buffer
				int lineend = -1;
				for (int i = 0; i < len; i++)
				{
					if (data[i] == '\n')
					{
						lineend = i;
						break;
					}
				}

				// if there is no line, abor.
				if ( lineend < 0 )
				{
					return false;
				}

				// read the line
				len = recv(m_socket, data, lineend + 1, 0);
				data[lineend] = '\0';

				// parse data
				parseData(data, lineend);

				return true;
			break;
		}

		return false;
	}

	void GPSConnector::parseData(char *data, unsigned int size)
	{
		// Example:
		// GPSD,O=RMC 1211456745.44 0.005 52.273185 10.525763
		// string gpsdata("GPSD,O=RMC 1211456745.44 0.005 52.273185 10.525763");
		string gpsdata(data);

		// split data
		vector<string> token = Utils::tokenize(" ", gpsdata);

		// at least 5 data fields
		if (token.size() < 5) return;

		// Prefix of the important line "GPSD,O="
		if (token[0].find("GPSD,O=") != 0) return;

		// read type
		string datatype = token[0].substr(7, 3);

		// read daten
		string time = token[1];
		string x = token[2];
		string latitude = token[3];
		string longitude = token[4];

		MutexLock l(m_datalock);
		m_time = strtod(time.c_str(), NULL);
		m_longitude = strtod(longitude.c_str(), NULL);
		m_latitude = strtod(latitude.c_str(), NULL);
	}

	void GPSConnector::initialize()
	{
	}

	void GPSConnector::terminate()
	{
		disconnect();
		setState( DISCONNECTED );
	}

	void GPSConnector::tick()
	{
		if (getState() == DISCONNECTED)
		{
			// reconnect
			if ( !connect() )
			{
				sleep(2);
				return;
			}
		}

		switch ( getState() )
		{
			case READY:
				if (readData())
				{
					m_datatimeout = 0;
				}
				else
				{
					m_datatimeout++;

					if (m_datatimeout > 1000)
					{
						setState(NO_GPS_DATA);
						m_datatimeout = 0;
					}
				}
			break;

			default:
				if (readData())
				{
					setState( READY );
				}
				else
				{
					m_datatimeout++;

					if (m_datatimeout > 1000)
					{
						disconnect();
						setState(DISCONNECTED);
						m_datatimeout = 0;
					}
				}

			break;
		}

		usleep(1000);
	}

	double GPSConnector::getTime()
	{
		MutexLock l(m_datalock);
		return m_time;
	}

	double GPSConnector::getLongitude()
	{
		MutexLock l(m_datalock);
		return m_longitude;
	}

	double GPSConnector::getLatitude()
	{
		MutexLock l(m_datalock);
		return m_latitude;
	}

	void GPSConnector::setState(GPSState state)
	{
		if (state != getState())
		{
			MutexLock l(m_datalock);
			m_state = state;
		}
	}

	GPSState GPSConnector::getState()
	{
		MutexLock l(m_datalock);
		return m_state;
	}
}
