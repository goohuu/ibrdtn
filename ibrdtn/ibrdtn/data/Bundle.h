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
#include "ibrcommon/refcnt_ptr.h"
#include <ostream>
#include <cassert>
#include <set>
#include <map>

namespace dtn
{
	namespace data
	{
		class ExtensionBlockFactory;

		class Bundle : public PrimaryBlock
		{
			friend class DefaultSerializer;
			friend class DefaultDeserializer;

		public:
			static std::map<char, ExtensionBlockFactory*>& getExtensionBlockFactories();

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

			public:
				BlockList();
				~BlockList();

				void push_front(Block *block);
				void push_back(Block *block);
				void insert(Block *block, const Block *before);
				void remove(const Block *block);
				void clear();

				const std::set<dtn::data::EID> getEIDs() const;

				template<typename T> T& get();
				template<typename T> const T& get() const;

				template<typename T>
				const std::list<const T*> getList() const;

				const std::list<const Block*> getList() const;

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

			template<typename T>
			T& getBlock();

			template<typename T>
			const T& getBlock() const;

			template<typename T>
			const std::list<const T*> getBlocks() const;

			template<typename T>
			T& push_front();

			template<typename T>
			T& push_back();

			template<typename T>
			T& insert(const dtn::data::Block &before);

			dtn::data::PayloadBlock& push_front(ibrcommon::BLOB::Reference &ref);
			dtn::data::PayloadBlock& push_back(ibrcommon::BLOB::Reference &ref);
			dtn::data::PayloadBlock& insert(const dtn::data::Block &before, ibrcommon::BLOB::Reference &ref);

			dtn::data::Block& push_back(dtn::data::ExtensionBlockFactory &factory);

			void remove(const dtn::data::Block &block);
			void clearBlocks();

			string toString() const;

		private:
			BlockList _blocks;
		};

		template<typename T>
		const std::list<const T*> Bundle::getBlocks() const
		{
			return _blocks.getList<T>();
		}

		template<typename T>
		T& Bundle::getBlock()
		{
			return _blocks.get<T>();
		}

		template<typename T>
		const T& Bundle::getBlock() const
		{
			return _blocks.get<T>();
		}

		template<typename T>
		const T& Bundle::BlockList::get() const
		{
			// copy all blocks to the list
			for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				if ((*iter)->_blocktype == T::BLOCK_TYPE)
				{
					const Block *b = (*iter).getPointer();
					return static_cast<const T&>(*b);
				}
			}

			throw NoSuchBlockFoundException();
		}

		template<typename T>
		T& Bundle::BlockList::get()
		{
			// copy all blocks to the list
			for (std::list<refcnt_ptr<Block> >::iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				if ((*iter)->_blocktype == T::BLOCK_TYPE)
				{
					T *b = dynamic_cast<T*>((*iter).getPointer());
					if (b != NULL)
					{
						return (*b);
					}
				}
			}

			throw NoSuchBlockFoundException();
		}

		template<typename T>
		const std::list<const T*> Bundle::BlockList::getList() const
		{
			// create a list of blocks
			std::list<const T*> ret;

			// copy all blocks to the list
			for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				if ((*(*iter))._blocktype == T::BLOCK_TYPE)
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

		template<typename T>
		T& Bundle::push_front()
		{
			T *tmpblock = new T();
			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);
			assert(block != NULL);
			_blocks.push_front(block);
			return (*tmpblock);
		}

		template<typename T>
		T& Bundle::push_back()
		{
			T *tmpblock = new T();
			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);
			assert(block != NULL);
			_blocks.push_back(block);
			return (*tmpblock);
		}

		template<typename T>
		T& Bundle::insert(const dtn::data::Block &before)
		{
			T *tmpblock = new T(*this);
			dtn::data::Block *block = dynamic_cast<dtn::data::Block*>(tmpblock);
			assert(block != NULL);
			_blocks.insert(block, &before);
			return (*tmpblock);
		}
	}
}

#endif /* BUNDLE_H_ */
