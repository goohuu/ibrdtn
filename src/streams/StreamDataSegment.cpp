/*
 * StreamDataSegment.cpp
 *
 *  Created on: 30.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamDataSegment.h"
#include <stdlib.h>
#include <typeinfo>
#include "ibrdtn/streams/bpstreambuf.h"

using namespace std;

namespace dtn
{
	namespace streams
	{
		StreamDataSegment::StreamDataSegment(SegmentType type, size_t size)
		 : _value(size), _type(type), _reason(MSG_SHUTDOWN_NONE), _flags(0)
		{
		}

		StreamDataSegment::StreamDataSegment()
		: _value(0), _type(MSG_KEEPALIVE), _reason(MSG_SHUTDOWN_NONE), _flags(0)
		{
		}

		StreamDataSegment::StreamDataSegment(ShutdownReason reason, size_t reconnect)
		: _value(reconnect), _type(MSG_SHUTDOWN), _reason(MSG_SHUTDOWN_NONE), _flags(0)
		{
		}

		StreamDataSegment::~StreamDataSegment()
		{
		}

		std::ostream &operator<<(std::ostream &stream, const StreamDataSegment &seg)
		{
			try {
				// do we write to a bpstreambuf?
				bpstreambuf &buf = dynamic_cast<bpstreambuf&>(*stream.rdbuf());

				// write seg
				buf << seg;
			} catch (std::bad_cast ex) {
				// write seg to "stream"
				seg.write(stream);
			}
		}

		std::istream &operator>>(std::istream &stream, StreamDataSegment &seg)
		{
			try {
				// do we write to a bpstreambuf?
				bpstreambuf &buf = dynamic_cast<bpstreambuf&>(*stream.rdbuf());

				// read seg
				buf >> seg;
			} catch (std::bad_cast ex) {
				// read seg from "stream"
				seg.read(stream);
			}
		}

		void StreamDataSegment::read( std::istream &stream )
		{
			dtn::data::SDNV value;

			char header;
			stream.get(header);

			_type = SegmentType( (header & 0xF0) >> 4 );
			_flags = (header & 0x0F);

			switch (_type)
			{
			case MSG_DATA_SEGMENT:
				// read the length
				stream >> value;
				_value = value.getValue();
				break;

			case MSG_ACK_SEGMENT:
				// read the acknowledged length
				stream >> value;
				_value = value.getValue();
				break;

			case MSG_REFUSE_BUNDLE:
				break;

			case MSG_KEEPALIVE:
				break;

			case MSG_SHUTDOWN:
				// read the reason (char) + reconnect time (SDNV)
				char reason;
				stream.get(reason);
				_reason = ShutdownReason(reason);

				stream >> value;
				_value = value.getValue();
				break;
			}
		}

		void StreamDataSegment::write( std::ostream &stream ) const
		{
			char header = _flags;
			header += ((_type & 0x0F) << 4);

			// write the header (1-byte)
			stream.put(header);

			dtn::data::SDNV value(_value);

			switch (_type)
			{
			case MSG_DATA_SEGMENT:
				// write the length + data
				stream << value;
				break;

			case MSG_ACK_SEGMENT:
				// write the acknowledged length
				stream << value;
				break;

			case MSG_REFUSE_BUNDLE:
				break;

			case MSG_KEEPALIVE:
				break;

			case MSG_SHUTDOWN:
				// write the reason (char) + reconnect time (SDNV)
				stream.put((char)_reason);
				stream << value;
				break;
			}
		}
	}
}
