/*
 * StreamDataSegment.h
 *
 *  Created on: 30.06.2009
 *      Author: morgenro
 */


#ifndef STREAMDATASEGMENT_H_
#define STREAMDATASEGMENT_H_

#include "ibrdtn/default.h"
#include "ibrdtn/streams/BundleWriter.h"
#include "ibrdtn/streams/BundleStreamReader.h"

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		class StreamDataSegment
		{
		public:
			enum SegmentType
			{
				MSG_DATA_SEGMENT = 0x1,
				MSG_ACK_SEGMENT = 0x2,
				MSG_REFUSE_BUNDLE = 0x3,
				MSG_KEEPALIVE = 0x4,
				MSG_SHUTDOWN = 0x5
			};

			enum ShutdownReason
			{
				MSG_SHUTDOWN_NONE = 0x00,
				MSG_SHUTDOWN_IDLE_TIMEOUT = 0x01,
				MSG_SHUTDOWN_VERSION_MISSMATCH = 0x02,
				MSG_SHUTDOWN_BUSY = 0x03
			};

			StreamDataSegment(SegmentType type, size_t size); // creates a ACK or DATA segment
			StreamDataSegment(); // Creates a Keep-Alive Message
			StreamDataSegment(ShutdownReason reason, size_t reconnect = 0);  // Creates a Shutdown Message

			virtual ~StreamDataSegment();

			size_t _value;
			SegmentType _type;
			ShutdownReason _reason;
			u_int8_t _flags;

			friend std::ostream &operator<<(std::ostream &stream, const StreamDataSegment &seg);
			friend std::istream &operator>>(std::istream &stream, StreamDataSegment &seg);

		private:
			void write( std::ostream &stream ) const;
			void read( std::istream &stream );
		};
	}
}

#endif /* STREAMDATASEGMENT_H_ */
