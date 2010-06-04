/*
 * Bundle.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/ExtensionBlock.h"
#include "ibrdtn/data/ExtensionBlockFactory.h"
#include "ibrdtn/data/Serializer.h"

namespace dtn
{
	namespace data
	{
		std::map<char, ExtensionBlockFactory*>& Bundle::getExtensionBlockFactories()
		{
			static std::map<char, ExtensionBlockFactory*> factories;
			return factories;
		}

		Bundle::Bundle()
		{
		}

		Bundle::~Bundle()
		{
			clearBlocks();
		}

		Bundle::BlockList::BlockList()
		{
		}

		Bundle::BlockList::~BlockList()
		{
		}

		void Bundle::BlockList::append(Block *block)
		{
			// set the last block flag
			block->_procflags |= dtn::data::Block::LAST_BLOCK;

			if (_blocks.size() > 0)
			{
				// remove the last block flag of the previous block
				dtn::data::Block *lastblock = (_blocks.back().getPointer());
				lastblock->set(dtn::data::Block::LAST_BLOCK, false);
			}

			_blocks.push_back(refcnt_ptr<Block>(block));
		}

		void Bundle::BlockList::insert(Block *block, const Block *before)
		{

		}

		void Bundle::BlockList::remove(const Block *block)
		{
			// delete all blocks
			for (std::list<refcnt_ptr<Block> >::iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				const dtn::data::Block &lb = (*(*iter));
				if ( &lb == block )
				{
					_blocks.erase(iter++);

					// set the last block bit
					(*_blocks.back()).set(dtn::data::Block::LAST_BLOCK, true);

					return;
				}
			}
		}

		void Bundle::BlockList::clear()
		{
			// clear the list of objects
			_blocks.clear();
		}

		const std::list<const Block*> Bundle::BlockList::getList() const
		{
			std::list<const dtn::data::Block*> ret;

			for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				ret.push_back( (*iter).getPointer() );
			}

			return ret;
		}

		const std::set<dtn::data::EID> Bundle::BlockList::getEIDs() const
		{
			std::set<dtn::data::EID> ret;

			for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				std::list<EID> elist = (*iter)->getEIDList();

				for (std::list<dtn::data::EID>::const_iterator iter = elist.begin(); iter != elist.end(); iter++)
				{
					ret.insert(*iter);
				}
			}

			return ret;
		}

		bool Bundle::operator!=(const Bundle& other) const
		{
			return PrimaryBlock(*this) != other;
		}

		bool Bundle::operator==(const Bundle& other) const
		{
			return PrimaryBlock(*this) == other;
		}

		bool Bundle::operator<(const Bundle& other) const
		{
			return PrimaryBlock(*this) < other;
		}

		bool Bundle::operator>(const Bundle& other) const
		{
			return PrimaryBlock(*this) > other;
		}

		const std::list<const dtn::data::Block*> Bundle::getBlocks() const
		{
			return _blocks.getList();
		}

		void Bundle::removeBlock(const dtn::data::Block &block)
		{
			_blocks.remove(&block);
		}

		void Bundle::clearBlocks()
		{
			_blocks.clear();
		}

		dtn::data::Block& Bundle::appendBlock(dtn::data::ExtensionBlockFactory &factory)
		{
			dtn::data::Block *block = factory.create();
			assert(block != NULL);
			_blocks.append(block);
			return (*block);
		}

		dtn::data::PayloadBlock& Bundle::appendPayloadBlock(ibrcommon::BLOB::Reference &ref)
		{
			dtn::data::PayloadBlock *tmpblock = new dtn::data::PayloadBlock(ref);
			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);
			assert(block != NULL);
			_blocks.append(block);
			return (*tmpblock);
		}

//		dtn::data::PayloadBlock& Bundle::insertPayloadBlock(dtn::data::Block &before, ibrcommon::BLOB::Reference &ref)
//		{
//			dtn::data::PayloadBlock *tmpblock = new dtn::data::PayloadBlock(*this, ref);
//			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);
//			assert(block != NULL);
//
//			_blocks.push_front(block);
//			return (*tmpblock);
//		}

		string Bundle::toString() const
		{
			return PrimaryBlock::toString();
		}

//		std::ostream &operator<<(std::ostream &stream, const dtn::data::Bundle &b)
//		{
//			DefaultSerializer(stream) << b;
//			return stream;
//		}
//
//		std::istream &operator>>(std::istream &stream, dtn::data::Bundle &b)
//		{
//			DefaultDeserializer(stream) >> b;
//			return stream;
//		}
	}
}

