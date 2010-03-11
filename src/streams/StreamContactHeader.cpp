/*
 * StreamContactHeader.cpp
 *
 *  Created on: 30.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/BundleString.h"
#include "ibrdtn/streams/bpstreambuf.h"
#include <typeinfo>

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		StreamContactHeader::StreamContactHeader()
		 : _flags(1), _keepalive(0)
		{
		}

		StreamContactHeader::StreamContactHeader(EID localeid)
		 : _localeid(localeid), _flags(1), _keepalive(0)
		{
		}

		StreamContactHeader::~StreamContactHeader()
		{
		}

		StreamContactHeader& StreamContactHeader::operator=(const StreamContactHeader &other)
		{
			_localeid = other._localeid;
			_flags = other._flags;
			_keepalive = other._keepalive;
			return *this;
		}

		const EID StreamContactHeader::getEID() const
		{
			return _localeid;
		}

		std::ostream &operator<<(std::ostream &stream, const StreamContactHeader &h)
		{
			try {
				// do we write to a bpstreambuf?
				bpstreambuf &buf = dynamic_cast<bpstreambuf&>(*stream.rdbuf());

				// write header to the buffer
				buf << h;
			} catch (std::bad_cast ex) {
				// write seg to "stream"
				h.write(stream);
			}
		}

		std::istream &operator>>(std::istream &stream, StreamContactHeader &h)
		{
			try {
				// do we write to a bpstreambuf?
				bpstreambuf &buf = dynamic_cast<bpstreambuf&>(*stream.rdbuf());

				// read seg
				buf >> h;
			} catch (std::bad_cast ex) {
				// read seg from "stream"
				h.read(stream);
			}
		}

		void StreamContactHeader::read( std::istream &stream )
		{
			char buffer[512];
			char magic[5];

			// wait for magic
			stream.read(magic, 4); magic[4] = '\0';
			string str_magic(magic);

			if (str_magic != "dtn!")
			{
				throw exceptions::InvalidDataException("not talking dtn");
			}

			// version
			char version; stream.get(version);
			if (version != TCPCL_VERSION)
			{
				throw exceptions::InvalidDataException("invalid bundle protocol version");
			}

			// flags
			stream.get(_flags);

			// keepalive
			char keepalive[2];
			stream.read(keepalive, 2);

			_keepalive = 0;
			_keepalive += keepalive[0];
			_keepalive += (keepalive[1] << 8);

			dtn::data::BundleString eid;
			stream >> eid;
			_localeid = EID(eid);
		}

		void StreamContactHeader::write( std::ostream &stream ) const
		{
			//BundleStreamWriter writer(stream);
			stream << "dtn!" << TCPCL_VERSION << _flags;
			stream.write( (char*)&_keepalive, 2 );

			dtn::data::BundleString eid(_localeid.getString());
			stream << eid;
		}
	}
}
