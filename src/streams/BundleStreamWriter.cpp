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

		size_t BundleStreamWriter::getSizeOf(u_int64_t value)
		{
			return dtn::data::SDNV::getLength(value);
		}

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

		size_t BundleStreamWriter::writeSDNV(u_int64_t value)
		{
			dtn::data::SDNV sdnv(value);
			_output << sdnv;
			return sdnv.getLength();
		}

		size_t BundleStreamWriter::write(dtn::data::SDNV value) {	return writeSDNV(value.getValue()); 	}
		size_t BundleStreamWriter::write(unsigned int value) {	return writeSDNV(value); 	}
		size_t BundleStreamWriter::write(u_int64_t value) 	 {	return writeSDNV(value); 	}
		size_t BundleStreamWriter::write(int value) 		 {	return writeSDNV(value); 	}

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

//		size_t BundleStreamWriter::write(size_t value)
//		{
//		  return write((char*)&value, sizeof(size_t));
//		}

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

		size_t BundleStreamWriter::write(istream &input)
		{
			// write data from stream to stream
			char data[512];
			size_t size = 1;
			size_t len = 0;

			while (size > 0)
			{
				size = input.readsome(data, 512);
				len += write(data, size);
			}

			return len;
		}

		size_t BundleStreamWriter::write(dtn::blob::BLOBReference &ref)
		{
			size_t len = ref.read(_output);
			return len;
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
			u_int64_t length =
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