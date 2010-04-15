/*
 * StreamContactHeader.h
 *
 *  Created on: 30.06.2009
 *      Author: morgenro
 */



#ifndef STREAMCONTACTHEADER_H_
#define STREAMCONTACTHEADER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/EID.h"

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		static const unsigned char TCPCL_VERSION = 3;

		class StreamContactHeader
		{
		public:
			StreamContactHeader();
			StreamContactHeader(EID localeid);
			virtual ~StreamContactHeader();

			StreamContactHeader& operator=(const StreamContactHeader &other);

			const EID getEID() const;

			EID _localeid;
			char _flags;
			u_int16_t _keepalive;

			friend std::ostream &operator<<(std::ostream &stream, const StreamContactHeader &h);
			friend std::istream &operator>>(std::istream &stream, StreamContactHeader &h);
		};
	}
}

#endif /* STREAMCONTACTHEADER_H_ */
