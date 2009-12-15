/*
 * StreamDataSegment.cpp
 *
 *  Created on: 30.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamDataSegment.h"
#include <stdlib.h>

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

		size_t StreamDataSegment::read( BundleStreamReader &reader )
		{
			size_t ret = 0;

			char header; ret += reader.readChar(header);

			_type = SegmentType( (header & 0xF0) >> 4 );
			_flags = (header & 0x0F);

			switch (_type)
			{
			case MSG_DATA_SEGMENT:
				// write the length + data
				ret += reader.readSDNV(_value);
				break;

			case MSG_ACK_SEGMENT:
				// write the acknowledged length
				ret += reader.readSDNV(_value);
				break;

			case MSG_REFUSE_BUNDLE:
				break;

			case MSG_KEEPALIVE:
				break;

			case MSG_SHUTDOWN:
				// write the reason (char) + reconnect time (SDNV)
				char reason;
				ret += reader.readChar(reason);
				_reason = ShutdownReason(reason);

				ret += reader.readSDNV(_value);
				break;
			}

			return ret;
		}

//		size_t StreamDataSegment::read( BundleStreamReader &reader )
//		{
//			size_t ret = 0;
//
//			// read the header first
//			ret += readHeader(reader);
//
////			if (_data != NULL) free(_data); _data = NULL;
////
////			switch (_type)
////			{
////			case MSG_DATA_SEGMENT:
////				_data = (char*)calloc(_value, sizeof(char));
////				reader.read(_data, _value); ret += _value;
////				break;
////
////			default:
////				break;
////			}
//
//			return ret;
//		}

		size_t StreamDataSegment::write( BundleWriter &writer ) const
		{
			size_t ret = 0;

			char header = _flags;
			header += ((_type & 0x0F) << 4);

			// write the header (1-byte)
			ret += writer.write(header);

			switch (_type)
			{
			case MSG_DATA_SEGMENT:
				// write the length + data
				ret += writer.write(_value);
				//ret += writer.write(_data, _value);
				break;

			case MSG_ACK_SEGMENT:
				// write the acknowledged length
				ret += writer.write(_value);
				break;

			case MSG_REFUSE_BUNDLE:
				break;

			case MSG_KEEPALIVE:
				break;

			case MSG_SHUTDOWN:
				// write the reason (char) + reconnect time (SDNV)
				ret += writer.write((char)_reason);
				ret += writer.write(_value);
				break;
			}

			return ret;
		}
	}
}
