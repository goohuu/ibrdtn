/*
 * DTNTime.h
 *
 *  Created on: 15.12.2009
 *      Author: morgenro
 */

#ifndef DTNTIME_H_
#define DTNTIME_H_

#include "ibrdtn/data/SDNV.h"

namespace dtn
{
	namespace data
	{
		class DTNTime
		{
		public:
			DTNTime();
			DTNTime(SDNV seconds, SDNV nanoseconds);
			~DTNTime();

			SDNV getTimestamp();

			size_t decode(const char *data, const size_t len);

			/**
			 * set the DTNTime to the current time
			 */
			void set();

			void operator+=(const size_t value);

		private:
			friend std::ostream &operator<<(std::ostream &stream, const dtn::data::DTNTime &obj);
			friend std::istream &operator>>(std::istream &stream, dtn::data::DTNTime &obj);

			SDNV _seconds;
			SDNV _nanoseconds;
		};
	}
}


#endif /* DTNTIME_H_ */
