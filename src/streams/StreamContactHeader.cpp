/*
 * StreamContactHeader.cpp
 *
 *  Created on: 30.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Exceptions.h"

using namespace dtn::data;

namespace dtn
{
	namespace streams
	{
		StreamContactHeader::StreamContactHeader()
		 : _flags(0), _keepalive(0)
		{
		}

		StreamContactHeader::StreamContactHeader(EID localeid)
		 : _localeid(localeid), _flags(0), _keepalive(0)
		{
		}

		StreamContactHeader::~StreamContactHeader()
		{
		}

		const EID StreamContactHeader::getEID() const
		{
			return _localeid;
		}

		void StreamContactHeader::read(dtn::streams::BundleStreamReader &reader)
		//std::istream &operator>>(std::istream &stream, dtn::streams::StreamContactHeader &obj)
		{
			//BundleStreamReader reader(stream);

			size_t ret = 0;
			char buffer[512];

			// wait for magic
			reader.read(buffer, 4);
			string magic; magic.assign(buffer, 4);
			if (magic != "dtn!")
			{
				throw exceptions::InvalidDataException("not talking dtn");
			}

			// version
			char version; reader.readChar(version);
			if (version != dtn::data::BUNDLE_VERSION)
			{
				throw exceptions::InvalidDataException("invalid bundle protocol version");
			}

			ret = 5;

			// flags
			ret += reader.readChar(_flags);


			// keepalive
			char keepalive[2];
			ret += reader.readChar(keepalive[0]);
			ret += reader.readChar(keepalive[1]);
			_keepalive = 0;
			_keepalive += keepalive[0];
			_keepalive += (keepalive[1] << 8);

			// length of eid
			size_t length; ret += reader.readSDNV(length);

			// eid
			reader.read(buffer, length); ret += length;
			string eid; eid.assign(buffer, length);
			_localeid = EID(eid);

			//return stream;
		}

		void StreamContactHeader::write(dtn::streams::BundleWriter &writer) const
		//std::ostream &operator<<(std::ostream &stream, const dtn::streams::StreamContactHeader &obj)
		{
			//BundleStreamWriter writer(stream);
			writer.write("dtn!");
			writer.write(BUNDLE_VERSION);
			writer.write(_flags);
			writer.write((char*)&_keepalive, 2);

			string eid = _localeid.getString();
			writer.write(eid.length());
			writer.write(eid);

			//return stream;
		}
	}
}