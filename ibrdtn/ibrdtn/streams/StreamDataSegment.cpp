/*
 * StreamDataSegment.cpp
 *
 *  Created on: 30.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamDataSegment.h"
#include "ibrdtn/data/SDNV.h"

using namespace std;

namespace dtn
{
	namespace streams
	{
		StreamDataSegment::StreamDataSegment(SegmentType type, size_t size)
		 : _value(size), _type(type), _reason(MSG_SHUTDOWN_IDLE_TIMEOUT), _flags(0)
		{
		}

		StreamDataSegment::StreamDataSegment(SegmentType type)
		: _value(0), _type(type), _reason(MSG_SHUTDOWN_IDLE_TIMEOUT), _flags(0)
		{
		}

		StreamDataSegment::StreamDataSegment(ShutdownReason reason, size_t reconnect)
		: _value(reconnect), _type(MSG_SHUTDOWN), _reason(reason), _flags(3)
		{
		}

		StreamDataSegment::~StreamDataSegment()
		{
		}

		std::ostream &operator<<(std::ostream &stream, const StreamDataSegment &seg)
		{
			char header = seg._flags;
			header += ((seg._type & 0x0F) << 4);

			// write the header (1-byte)
			stream.put(header);

			switch (seg._type)
			{
			case StreamDataSegment::MSG_DATA_SEGMENT:
				// write the length + data
				stream << dtn::data::SDNV(seg._value);
				break;

			case StreamDataSegment::MSG_ACK_SEGMENT:
				// write the acknowledged length
				stream << dtn::data::SDNV(seg._value);
				break;

			case StreamDataSegment::MSG_REFUSE_BUNDLE:
				break;

			case StreamDataSegment::MSG_KEEPALIVE:
				break;

			case StreamDataSegment::MSG_SHUTDOWN:
				// write the reason (char) + reconnect time (SDNV)
				stream.put((char)seg._reason);
				stream << dtn::data::SDNV(seg._value);
				break;
			}

			return stream;
		}

		std::istream &operator>>(std::istream &stream, StreamDataSegment &seg)
		{
			dtn::data::SDNV value;

			char header = 0;
			stream.get(header);

			seg._type = StreamDataSegment::SegmentType( (header & 0xF0) >> 4 );
			seg._flags = (header & 0x0F);

			switch (seg._type)
			{
			case StreamDataSegment::MSG_DATA_SEGMENT:
				// read the length
				stream >> value;
				seg._value = value.getValue();
				break;

			case StreamDataSegment::MSG_ACK_SEGMENT:
				// read the acknowledged length
				stream >> value;
				seg._value = value.getValue();
				break;

			case StreamDataSegment::MSG_REFUSE_BUNDLE:
				break;

			case StreamDataSegment::MSG_KEEPALIVE:
				break;

			case StreamDataSegment::MSG_SHUTDOWN:
				// read the reason (char) + reconnect time (SDNV)
				char reason;
				stream.get(reason);
				seg._reason = StreamDataSegment::ShutdownReason(reason);

				stream >> value;
				seg._value = value.getValue();
				break;
			}

			return stream;
		}
	}
}
