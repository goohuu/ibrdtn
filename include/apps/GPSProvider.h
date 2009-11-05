#ifndef GPSPROVIDER_H_
#define GPSPROVIDER_H_

namespace emma
{
	enum GPSState
	{
		DISCONNECTED = 0,
		CONNECTED = 1,
		READY = 2,
		NO_GPS_DATA = 3
	};

	class GPSProvider
	{
	public:
		GPSProvider() {};
		virtual ~GPSProvider() {};

		virtual double getTime() = 0;
		virtual double getLongitude() = 0;
		virtual double getLatitude() = 0;

		virtual GPSState getState() = 0;
	};
}

#endif /*GPSPROVIDER_H_*/
