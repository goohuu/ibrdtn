#ifndef GPSDUMMY_H_
#define GPSDUMMY_H_

#include "emma/GPSProvider.h"

namespace emma
{
	class GPSDummy : public GPSProvider
	{
	public:
		GPSDummy(double latitude, double longitude);
		~GPSDummy();

		double getTime();
		double getLongitude();
		double getLatitude();

		GPSState getState();

	private:
		double m_latitude;
		double m_longitude;
	};
}

#endif /*GPSDUMMY_H_*/
