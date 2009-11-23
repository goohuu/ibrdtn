/*
 * CustodySignalBlock.cpp
 *
 *  Created on: 05.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include <stdlib.h>
#include <sstream>

namespace dtn
{
	namespace data
	{
		CustodySignalBlock::CustodySignalBlock()
		 : PayloadBlock(blob::BLOBManager::BLOB_MEMORY), _admfield(32), _status(0), _fragment_offset(0),
		 _fragment_length(0), _timeofsignal(0), _bundle_timestamp(0), _unknown(0),
		 _bundle_sequence(0)
		{
		}

		CustodySignalBlock::CustodySignalBlock(Block *block)
		 : PayloadBlock(block->getBLOBReference()), _admfield(32), _status(0), _fragment_offset(0),
		 _fragment_length(0), _timeofsignal(0), _bundle_timestamp(0), _unknown(0),
		 _bundle_sequence(0)
		{
			read();
		}

		CustodySignalBlock::CustodySignalBlock(BLOBReference ref)
		 : PayloadBlock(ref), _admfield(32), _status(0), _fragment_offset(0),
		 _fragment_length(0), _timeofsignal(0), _bundle_timestamp(0), _unknown(0),
		 _bundle_sequence(0)
		{
		}

		CustodySignalBlock::~CustodySignalBlock()
		{
		}

		void CustodySignalBlock::read()
		{
			// read the attributes out of the BLOBReference
			BLOBReference ref = Block::getBLOBReference();
			size_t remain = ref.getSize();
			char buffer[remain];

			// read the data into the buffer
			ref.read(buffer, 0, remain);

			// make a useable pointer
			char *data = buffer;

			SDNV value = 0;
			size_t len = 0;

			_admfield = data[0]; 	remain--; data++;
			_status = data[0]; 		remain--; data++;

			if ( _admfield & 0x01 )
			{
				len = _fragment_offset.decode(data, remain); remain -= len; data += len;
				len = _fragment_length.decode(data, remain); remain -= len; data += len;
			}

			len = _timeofsignal.decode(data, remain); 		remain -= len; data += len;
			len = _bundle_timestamp.decode(data, remain); 	remain -= len; data += len;
			len = _unknown.decode(data, remain); 			remain -= len; data += len;
			len = _bundle_sequence.decode(data, remain); 	remain -= len; data += len;
			len = value.decode(data, remain); 				remain -= len; data += len;

			string source;
			source.assign(data, remain);
			_source = EID(source);
		}

		void CustodySignalBlock::commit()
		{
			stringstream ss;
			dtn::streams::BundleStreamWriter w(ss);

			w.write(_admfield);
			w.write(_status);

			if ( _admfield & 0x01 )
			{
				w.write(_fragment_offset);
				w.write(_fragment_length);
			}

			w.write(_timeofsignal);
			w.write(_bundle_timestamp);
			w.write(_unknown);
			w.write(_bundle_sequence);
			w.write(_source.getString().length());
			w.write(_source.getString());

			// clear the blob
			BLOBReference ref = Block::getBLOBReference();
			ref.clear();

			// copy the content of ss to the blob
			string data = ss.str();
			ref.append(data.c_str(), data.length());
		}

		void CustodySignalBlock::setMatch(const Bundle& other)
		{
			// set bundle parameter
			if (other._procflags & Bundle::FRAGMENT)
			{
				_fragment_offset = other._fragmentoffset;
				_fragment_length = other._appdatalength;

				if (!(_admfield & 1)) _admfield += 1;
			}

			_bundle_timestamp = other._timestamp;
			_bundle_sequence = other._sequencenumber;
			_source = other._source;
		}

		bool CustodySignalBlock::match(const Bundle& other) const
		{
			if (_bundle_timestamp != other._timestamp) return false;
			if (_bundle_sequence != other._sequencenumber) return false;
			if (_source != other._source) return false;

			// set bundle parameter
			if (other._procflags & Bundle::FRAGMENT)
			{
				if (!(_admfield & 1)) return false;
				if (_fragment_offset != other._fragmentoffset) return false;
				if (_fragment_length != other._appdatalength) return false;
			}

			return true;
		}

	}
}
