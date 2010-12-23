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
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/StatusReportBlock.h"
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

				void push_front(Block *block);
				void push_back(Block *block);
				void insert(Block *block, const Block *before);
				void remove(const Block *block);
				void clear();

				const std::set<dtn::data::EID> getEIDs() const;

				template<class T> T& get();
				template<class T> const T& get() const;

				template<class T>
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

			dtn::data::Block& push_back(dtn::data::ExtensionBlock::Factory &factory);
			dtn::data::Block& insert(dtn::data::ExtensionBlock::Factory &factory, const dtn::data::Block &before);

			void remove(const dtn::data::Block &block);
			void clearBlocks();

			string toString() const;

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
		inline CustodySignalBlock& Bundle::BlockList::get<CustodySignalBlock>()
		{
			try {
				// copy all blocks to the list
				for (std::list<refcnt_ptr<Block> >::iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
				{
					if ((*iter)->getType() == PayloadBlock::BLOCK_TYPE)
					{
						Block *b = (*iter).getPointer();
						return dynamic_cast<CustodySignalBlock&>(*b);
					}
				}
			} catch (std::bad_cast) {

			}

			throw NoSuchBlockFoundException();
		}

		template<>
		inline const CustodySignalBlock& Bundle::BlockList::get<const CustodySignalBlock>() const
		{
			try {
				// copy all blocks to the list
				for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
				{
					if ((*iter)->getType() == PayloadBlock::BLOCK_TYPE)
					{
						const Block *b = (*iter).getPointer();
						return dynamic_cast<const CustodySignalBlock&>(*b);
					}
				}
			} catch (std::bad_cast) {

			}

			throw NoSuchBlockFoundException();
		}

		template<>
		inline StatusReportBlock& Bundle::BlockList::get<StatusReportBlock> ()
		{
			try {
				// copy all blocks to the list
				for (std::list<refcnt_ptr<Block> >::iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
				{
					if ((*iter)->getType() == PayloadBlock::BLOCK_TYPE)
					{
						Block *b = (*iter).getPointer();
						return dynamic_cast<StatusReportBlock&>(*b);
					}
				}
			} catch (std::bad_cast) {

			}

			throw NoSuchBlockFoundException();
		}

		template<>
		inline const StatusReportBlock& Bundle::BlockList::get<const StatusReportBlock>() const
		{
			try {
				// copy all blocks to the list
				for (std::list<refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
				{
					if ((*iter)->getType() == PayloadBlock::BLOCK_TYPE)
					{
						const Block *b = (*iter).getPointer();
						return dynamic_cast<const StatusReportBlock&>(*b);
					}
				}
			} catch (std::bad_cast) {

			}

			throw NoSuchBlockFoundException();
		}

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
			} catch (std::bad_cast) {

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
			} catch (std::bad_cast) {

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
