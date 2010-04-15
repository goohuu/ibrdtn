/*
 * CustodySignalBlock.cpp
 *
 *  Created on: 05.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/BundleString.h"
#include <stdlib.h>
#include <sstream>

namespace dtn
{
	namespace data
	{
		CustodySignalBlock::CustodySignalBlock()
		 : PayloadBlock(ibrcommon::StringBLOB::create()), _admfield(32), _status(0), _fragment_offset(0),
		 _fragment_length(0), _timeofsignal(), _bundle_timestamp(0), _bundle_sequence(0)
		{
		}

		CustodySignalBlock::CustodySignalBlock(Block *block)
		 : PayloadBlock(block->getBLOB()), _admfield(32), _status(0), _fragment_offset(0),
		 _fragment_length(0), _timeofsignal(), _bundle_timestamp(0), _bundle_sequence(0)
		{
			read();
		}

		CustodySignalBlock::CustodySignalBlock(ibrcommon::BLOB::Reference ref)
		 : PayloadBlock(ref), _admfield(32), _status(0), _fragment_offset(0),
		 _fragment_length(0), _timeofsignal(), _bundle_timestamp(0), _bundle_sequence(0)
		{
		}

		CustodySignalBlock::~CustodySignalBlock()
		{
		}

		void CustodySignalBlock::read()
		{
			// read the attributes out of the BLOB
			ibrcommon::BLOB::Reference ref = Block::getBLOB();
			ibrcommon::MutexLock l(ref);

			(*ref) >> _admfield;
			(*ref) >> _status;

			if ( _admfield & 0x01 )
			{
				(*ref) >> _fragment_offset;
				(*ref) >> _fragment_length;
			}

			(*ref) >> _timeofsignal;
			(*ref) >> _bundle_timestamp;
			(*ref) >> _bundle_sequence;

			BundleString source;
			(*ref) >> source;
			_source = EID(source);
		}

		void CustodySignalBlock::commit()
		{
			// read the attributes out of the BLOB
			ibrcommon::BLOB::Reference ref = Block::getBLOB();
			ibrcommon::MutexLock l(ref);
			ref.clear();

			(*ref) << _admfield
				   << _status;

			if ( _admfield & 0x01 )
			{
				(*ref) << _fragment_offset
				   << _fragment_length;
			}

			(*ref) << _timeofsignal
			   << _bundle_timestamp
			   << _bundle_sequence
			   << BundleString(_source.getString());
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
