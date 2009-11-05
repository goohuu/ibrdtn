#include "emma/GPSDummy.h"
#include "ibrdtn/utils/Utils.h"

namespace emma
{
	GPSDummy::GPSDummy(double latitude, double longitude)
		:	m_latitude(latitude), m_longitude(longitude)
	{
	}

	GPSDummy::~GPSDummy()
	{
	}

	double GPSDummy::getTime()
	{
		return 0;
	}

	double GPSDummy::getLongitude()
	{
		return m_longitude;
	}

	double GPSDummy::getLatitude()
	{
		return m_latitude;
	}

	GPSState GPSDummy::getState()
	{
		return READY;
	}
}
