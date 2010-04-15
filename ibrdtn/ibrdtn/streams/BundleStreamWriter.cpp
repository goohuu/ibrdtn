/*
 * BundleStreamWriter.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/SDNV.h"
#include <stdlib.h>

namespace dtn
{
	namespace streams
	{
		BundleStreamWriter::BundleStreamWriter(ostream &output)
		 : _output(output)
		{
		}

		BundleStreamWriter::~BundleStreamWriter()
		{
		}

		size_t BundleStreamWriter::getSizeOf(size_t value) { return dtn::data::SDNV::getLength(value); }
//		size_t BundleStreamWriter::getSizeOf(u_int64_t value) { return dtn::data::SDNV::getLength(value); }
//		size_t BundleStreamWriter::getSizeOf(u_int32_t value) { return dtn::data::SDNV::getLength(value); }
//		size_t BundleStreamWriter::getSizeOf(u_int16_t value) { return dtn::data::SDNV::getLength(value); }

		size_t BundleStreamWriter::getSizeOf(const char value) { return 1; }
		size_t BundleStreamWriter::getSizeOf(unsigned char value) { return 1; }

		size_t BundleStreamWriter::getSizeOf(string value)
		{
			return value.length();
		}

		size_t BundleStreamWriter::getSizeOf(pair<size_t, size_t> value)
		{
			size_t size = 0;
			size += getSizeOf(value.first);
			size += getSizeOf(value.second);
			return size;
		}

		size_t BundleStreamWriter::write(const dtn::data::SDNV &value)
		{
			_output << value;
			return value.getLength();
		}

		size_t BundleStreamWriter::write(const size_t &value) {	return write(dtn::data::SDNV(value)); 	}
//		size_t BundleStreamWriter::write(const u_int64_t &value) {	return write(dtn::data::SDNV(value)); 	}
//		size_t BundleStreamWriter::write(const u_int32_t &value) {	return write(dtn::data::SDNV(value)); 	}
//		size_t BundleStreamWriter::write(const u_int16_t &value) {	return write(dtn::data::SDNV(value)); 	}

		size_t BundleStreamWriter::write(pair<size_t, size_t> value)
		{
			size_t len = 0;
			len += write(value.first);
			len += write(value.second);
			return len;
		}

		size_t BundleStreamWriter::write(double value)
		{
			return write((char*)&value, sizeof(double));
		}

		size_t BundleStreamWriter::write(char data)
		{
			return write((char*)&data, sizeof(char));

			_output << data;
			return sizeof(char);
		}

		size_t BundleStreamWriter::write(unsigned char data)
		{
			return write(&data, sizeof(char));
		}

		size_t BundleStreamWriter::write(unsigned char *data, size_t length)
		{
			_output.write((char*)data, length);
			return length;
		}

		size_t BundleStreamWriter::write(char *data, size_t length)
		{
			_output.write(data, length);
			return length;
		}

		size_t BundleStreamWriter::write(string value)
		{
			return write((char*)value.c_str(), value.length());
		}

		size_t BundleStreamWriter::write(istream &src)
		{
			size_t ret = 0;
			char buffer[512];

			while (src.good() && _output.good())
			{
				// read max. 512 bytes at once
				src.read(buffer, 512);

				// read bytes
				int bytes = src.gcount();

				// write buffer to the destination stream
				_output.write(buffer, bytes);

				// increment result
				ret += bytes;
			}

			return ret;
		}

		size_t BundleStreamWriter::write(const dtn::data::Bundle &b)
		{
			size_t len = 0;

			len += write(dtn::data::BUNDLE_VERSION);		// bundle version
			len += write(b._procflags);			// processing flags

			// create a dictionary
			dtn::data::Dictionary dict;

			// add own eids of the primary block
			dict.add(b._source);
			dict.add(b._destination);
			dict.add(b._reportto);
			dict.add(b._custodian);

			// add eids of attached blocks
			const list<dtn::data::Block*> blocks = b.getBlocks();
			list<dtn::data::Block*>::const_iterator iter = blocks.begin();

			while (iter != blocks.end())
			{
				dtn::data::Block *ref = (*iter);
				list<dtn::data::EID> eids = ref->getEIDList();
				dict.add( eids );
				iter++;
			}

			// predict the block length
			size_t length =
				getSizeOf( dict.getRef(b._destination) ) +
				getSizeOf( dict.getRef(b._source) ) +
				getSizeOf( dict.getRef(b._reportto) ) +
				getSizeOf( dict.getRef(b._custodian) ) +
				getSizeOf(b._timestamp) +
				getSizeOf(b._sequencenumber) +
				getSizeOf(b._lifetime) +
				getSizeOf( dict.getSize() ) +
				dict.getSize();

			if (b._procflags & dtn::data::Bundle::FRAGMENT)
			{
				length += getSizeOf(b._fragmentoffset) + getSizeOf(b._appdatalength);
			}

			len += write(length);				// block length

			/*
			 * write the ref block of the dictionary
			 * this includes scheme and ssp for destination, source, reportto and custodian.
			 */
			len += dict.writeRef( *this, b._destination );
			len += dict.writeRef( *this, b._source );
			len += dict.writeRef( *this, b._reportto );
			len += dict.writeRef( *this, b._custodian );

			len += write(b._timestamp);			// CREATION_TIMESTAMP
			len += write(b._sequencenumber);		// CREATION_TIMESTAMP_SEQUENCE
			len += write(b._lifetime);			// LIFETIME

			len += dict.writeLength( *this );		// DICTIONARY_LENGTH
			len += dict.writeByteArray( *this ); 	// DICTIONARY_BYTEARRAY

			if (b._procflags & dtn::data::Bundle::FRAGMENT)
			{
				len += write(b._fragmentoffset);	// FRAGMENTATION_OFFSET
				len += write(b._appdatalength);	// APPLICATION_DATA_LENGTH
			}

			/*
			 * write secondary blocks
			 */
			iter = blocks.begin();

			while (iter != blocks.end())
			{
				dtn::data::Block *ref = (*iter);
				len += ref->write( *this );
				iter++;
			}

			return len;
		}
	}
}
