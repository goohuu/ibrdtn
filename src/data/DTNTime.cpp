/*
 * DTNTime.cpp
 *
 *  Created on: 15.12.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/DTNTime.h"
#include "ibrdtn/utils/Utils.h"
#include <sys/time.h>

namespace dtn
{
	namespace data
	{
		DTNTime::DTNTime()
		 : _seconds(0), _nanoseconds(0)
		{
			set();
		}

		DTNTime::DTNTime(size_t seconds, size_t nanoseconds)
		 : _seconds(seconds), _nanoseconds(nanoseconds)
		{
		}

		DTNTime::DTNTime(SDNV seconds, SDNV nanoseconds)
		 : _seconds(seconds), _nanoseconds(nanoseconds)
		{
		}

		DTNTime::~DTNTime()
		{
		}

		void DTNTime::set()
		{
			_seconds = dtn::utils::Utils::get_current_dtn_time();
			timeval tv;

			if ( gettimeofday(&tv, NULL) )
			{
				_nanoseconds = SDNV(tv.tv_usec * 1000);
			}
		}

		SDNV DTNTime::getTimestamp()
		{
			return _seconds;
		}

		size_t DTNTime::decode(const char *data, const size_t len)
		{
			size_t ret = 0;

			ret += _seconds.decode(data, (len - ret));
			ret += _nanoseconds.decode(data, (len - ret));

			return ret;
		}

		void DTNTime::operator+=(const size_t value)
		{
			_seconds += value;
		}

		std::ostream& operator<<(std::ostream &stream, const dtn::data::DTNTime &obj)
		{
			stream << obj._seconds << obj._nanoseconds;
			return stream;
		}

		std::istream& operator>>(std::istream &stream, dtn::data::DTNTime &obj)
		{
			stream >> obj._seconds;
			stream >> obj._nanoseconds;
			return stream;
		}
	}
}
