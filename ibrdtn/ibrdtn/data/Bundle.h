/*
 * Bundle.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#ifndef BUNDLE_H_
#define BUNDLE_H_

#include "ibrdtn/data/Dictionary.h"
#include "ibrdtn/data/PrimaryBlock.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/ExtensionBlock.h"
#include "ibrcommon/refcnt_ptr.h"
#include <ostream>
#ifdef __DEVELOPMENT_ASSERTIONS__
#include <cassert>
#endif
#include <set>
#include <map>
#include <typeinfo>

namespace dtn
{
	namespace security
	{
		class StrictSerializer;
		class MutualSerializer;
	}

	namespace data
	{
		class CustodySignalBlock;
		class StatusReportBlock;

		class Bundle : public PrimaryBlock
		{
			friend class DefaultSerializer;
			friend class DefaultDeserializer;
			friend class dtn::security::StrictSerializer;
			friend class dtn::security::MutualSerializer;

		public:
			class NoSuchBlockFoundException : public ibrcommon::Exception
			{
				public:
					NoSuchBlockFoundException() : ibrcommon::Exception("No block found with this Block ID.")
					{
					};
			};

			class BlockList
			{
				friend class DefaultSerializer;
				friend class DefaultDeserializer;
				friend class dtn::security::StrictSerializer;
				friend class dtn::security::MutualSerializer;

			public:
				BlockList();
				virtual ~BlockList();

				BlockList& operator=(const BlockList &ref);

				void push_front(Block *block);
				void push_back(Block *block);
				void insert(Block *block, const Block *before);
				void remove(const Block *block);
				void clear();

				const std::set<dtn::data::EID> getEIDs() const;

				Block& get(int index);
				const Block& get(int index) const;
				template<class T> T& get();
				template<class T> const T& get() const;

				template<class T>
				const std::list<const T*> getList() const;

				const std::list<const Block*> getList() const;

				size_t size() const;

			private:
				std::list<refcnt_ptr<Block> > _blocks;
			};

			Bundle();
			virtual ~Bundle();

			bool operator==(const Bundle& other) const;
			bool operator!=(const Bundle& other) const;
			bool operator<(const Bundle& other) const;
			bool operator>(const Bundle& other) const;

			const std::list<const dtn::data::Block*> getBlocks() const;

			dtn::data::Block& getBlock(int index);
			const dtn::data::Block& getBlock(int index) const;

			template<class T>
			T& getBlock();

			template<class T>
			const T& getBlock() const;

			template<class T>
			const std::list<const T*> getBlocks() const;

			template<class T>
			T& push_front();

			template<class T>
			T& push_back();

			template<class T>
			T& insert(const dtn::data::Block &before);

			dtn::data::PayloadBlock& push_front(ibrcommon::BLOB::Reference &ref);
			dtn::data::PayloadBlock& push_back(ibrcommon::BLOB::Reference &ref);
			dtn::data::PayloadBlock& insert(const dtn::data::Block &before, ibrcommon::BLOB::Reference &ref);

			dtn::data::Block& push_front(dtn::data::ExtensionBlock::Factory &factory);
			dtn::data::Block& push_back(dtn::data::ExtensionBlock::Factory &factory);
			dtn::data::Block& insert(dtn::data::ExtensionBlock::Factory &factory, const dtn::data::Block &before);

			void remove(const dtn::data::Block &block);
			void clearBlocks();

			string toString() const;

			size_t blockCount() const;

		private:
			BlockList _blocks;
		};

		template<class T>
		const std::list<const T*> Bundle::getBlocks() const
		{
			return _blocks.getList<T>();
		}

		template<class T>
		T& Bundle::getBlock()
		{
			return _blocks.get<T>();
		}

		template<class T>
		const T& Bundle::getBlock() const
		{
			return _blocks.get<T>();
		}

		template<>
		CustodySignalBlock& Bundle::BlockList::get<CustodySignalBlock>();

		template<>
		const CustodySignalBlock& Bundle::BlockList::get<const CustodySignalBlock>() const;

		template<>
		StatusReportBlock& Bundle::BlockList::get<StatusReportBlock> ();

		template<>
		const StatusReportBlock& Bundle::BlockList::get<const StatusReportBlock>() const;

		template<class T>
		const T& Bundle::BlockList::get() const
		{
			try {
				// copy all blocks to the list
				for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
				{
					if ((*iter)->getType() == T::BLOCK_TYPE)
					{
						const Block *b = (*iter).getPointer();
						return dynamic_cast<const T&>(*b);
					}
				}
			} catch (const std::bad_cast&) {

			}

			throw NoSuchBlockFoundException();
		}

		template<class T>
		T& Bundle::BlockList::get()
		{
			try {
				// copy all blocks to the list
				for (std::list<refcnt_ptr<Block> >::iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
				{
					if ((*iter)->getType() == T::BLOCK_TYPE)
					{
						Block *b = (*iter).getPointer();
						return dynamic_cast<T&>(*b);
					}
				}
			} catch (const std::bad_cast&) {

			}

			throw NoSuchBlockFoundException();
		}

		template<class T>
		const std::list<const T*> Bundle::BlockList::getList() const
		{
			// create a list of blocks
			std::list<const T*> ret;

			// copy all blocks to the list
			for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				if ((*(*iter)).getType() == T::BLOCK_TYPE)
				{
					const T* obj = dynamic_cast<const T*>((*iter).getPointer());

					if (obj != NULL)
					{
						ret.push_back( obj );
					}
				}
			}

			return ret;
		}

		template<class T>
		T& Bundle::push_front()
		{
			T *tmpblock = new T();
			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);

#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(block != NULL);
#endif

			_blocks.push_front(block);
			return (*tmpblock);
		}

		template<class T>
		T& Bundle::push_back()
		{
			T *tmpblock = new T();
			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);

#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(block != NULL);
#endif

			_blocks.push_back(block);
			return (*tmpblock);
		}

		template<class T>
		T& Bundle::insert(const dtn::data::Block &before)
		{
			T *tmpblock = new T();
			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);

#ifdef __DEVELOPMENT_ASSERTIONS__
			assert(block != NULL);
#endif

			_blocks.insert(block, &before);
			return (*tmpblock);
		}
	}
}

#endif /* BUNDLE_H_ */
