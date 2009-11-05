/*
 * Bundle.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/utils/Utils.h"
#include "ibrdtn/streams/BundleStreamReader.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/streams/BundleCopier.h"

namespace dtn
{
	namespace data
	{
		size_t Bundle::__sequencenumber = 0;

		Bundle::Bundle()
		 : _procflags(0), _timestamp(0), _sequencenumber(0), _lifetime(3600), _fragmentoffset(0), _appdatalength(0)
		{
			_timestamp = utils::Utils::get_current_dtn_time();
			_sequencenumber = __sequencenumber;
			__sequencenumber++;
		}

		Bundle::~Bundle()
		{
		}

		bool Bundle::operator!=(const Bundle& other) const
		{
			return !((*this) == other);
		}

		bool Bundle::operator==(const Bundle& other) const
		{
			if (other._timestamp != _timestamp) return false;
			if (other._sequencenumber != _sequencenumber) return false;
			if (other._source != _source) return false;

			if (other._procflags & Bundle::FRAGMENT)
			{
				if (!(_procflags & Bundle::FRAGMENT)) return false;

				//if (other._fragmentoffset != _fragmentoffset) return false;
				if (other._appdatalength != _appdatalength) return false;
			}

			return true;
		}

		void Bundle::relabel()
		{
			_timestamp = utils::Utils::get_current_dtn_time();
			_sequencenumber = __sequencenumber;
			__sequencenumber++;
		}

		void Bundle::addBlock(Block *b)
		{
			if ((dynamic_cast<CustodySignalBlock*>(b) != NULL) || (dynamic_cast<StatusReportBlock*>(b) != NULL))
			{
				if (!(_procflags & Bundle::APPDATA_IS_ADMRECORD))
				{
					_procflags += Bundle::APPDATA_IS_ADMRECORD;
				}
			}

			// set the last block flag
			if (!(b->_procflags & 0x08)) b->_procflags += 0x08;

			if (_blocks.size() > 0)
			{
				// remove the last block flag of the previous block
				refcnt_ptr<Block> lastblock = _blocks.back();
				if (!(lastblock->_procflags & 0x08)) lastblock->_procflags -= 0x08;
			}

			refcnt_ptr<Block> ref(b);
			_blocks.push_back( ref );
		}

		const list<Block*> Bundle::getBlocks(char type) const
		{
			// create a list of blocks
			list<Block*> ret;

			// copy all blocks to the list
			list< refcnt_ptr<Block> >::const_iterator iter = _blocks.begin();

			while (iter != _blocks.end())
			{
				refcnt_ptr<Block> blockref = (*iter);
				if (blockref->_blocktype == type) ret.push_back( blockref.getPointer() );
				iter++;
			}

			return ret;
		}

		const list<Block*> Bundle::getBlocks() const
		{
			// create a list of blocks
			list<Block*> ret;

			// copy all blocks to the list
			list< refcnt_ptr<Block> >::const_iterator iter = _blocks.begin();

			while (iter != _blocks.end())
			{
				refcnt_ptr<Block> blockref = (*iter);
				ret.push_back( blockref.getPointer() );
				iter++;
			}

			return ret;
		}

		void Bundle::removeBlock(Block *b)
		{
		}

		void Bundle::clearBlocks()
		{
			// clear the list of objects
			_blocks.clear();
		}

		bool Bundle::isExpired() const
		{
			if (utils::Utils::get_current_dtn_time() > (_lifetime + _timestamp)) return true;
			return false;
		}

		string Bundle::toString() const
		{
			stringstream ss;
			ss << "[" << _timestamp << "." << _sequencenumber << "] " << _source.getString() << " -> " << _destination.getString() << " [size: " << getSize() << "]";
			return ss.str();
		}

		size_t Bundle::getSize() const
		{
			BundleStreamWriter writer(cout);
			size_t len = 0;

			len += writer.getSizeOf(BUNDLE_VERSION);		// bundle version
			len += writer.getSizeOf(_procflags);			// processing flags

			// create a dictionary
			Dictionary dict;

			// add own eids of the primary block
			dict.add(_source);
			dict.add(_destination);
			dict.add(_reportto);
			dict.add(_custodian);

			// add eids of attached blocks
			list< refcnt_ptr<Block> >::const_iterator iter = _blocks.begin();

			while (iter != _blocks.end())
			{
				refcnt_ptr<Block> ref = (*iter);
				list<EID> eids = ref->getEIDList();
				dict.add( eids );
				iter++;
			}

			// predict the block length
			u_int64_t length =
				writer.getSizeOf( dict.getRef(_destination) ) +
				writer.getSizeOf( dict.getRef(_source) ) +
				writer.getSizeOf( dict.getRef(_reportto) ) +
				writer.getSizeOf( dict.getRef(_custodian) ) +
				writer.getSizeOf(_timestamp) +
				writer.getSizeOf(_sequencenumber) +
				writer.getSizeOf(_lifetime) +
				writer.getSizeOf( dict.getSize() ) +
				dict.getSize();

			if (_procflags & Bundle::FRAGMENT)
			{
				length += writer.getSizeOf(_fragmentoffset) + writer.getSizeOf(_appdatalength);
			}

			/*
			 * secondary blocks
			 */
			iter = _blocks.begin();

			while (iter != _blocks.end())
			{
				refcnt_ptr<Block> ref = (*iter);
				length += ref->getSize();
				iter++;
			}

			return length;
		}

		std::ostream &operator<<(std::ostream &stream, const dtn::data::Bundle &b)
		{
			dtn::streams::BundleStreamWriter writer(stream);
			writer.write(b);
			return stream;
		}

		std::istream &operator>>(std::istream &stream, dtn::data::Bundle &b)
		{
			dtn::streams::BundleCopier copier(dtn::blob::BLOBManager::_instance, b);
			dtn::streams::BundleStreamReader reader(stream, copier);
			reader.readBundle();

			return stream;
		}
	}
}

