/*
 * Bundle.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#ifndef BUNDLE_H_
#define BUNDLE_H_

#include "ibrdtn/default.h"
#include "ibrdtn/streams/BundleWriter.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/data/Dictionary.h"
#include "ibrdtn/data/Block.h"
#include "ibrcommon/refcnt_ptr.h"
#include <ostream>

using namespace dtn::streams;

namespace dtn
{
	namespace data
	{
		static const unsigned char BUNDLE_VERSION = 0x06;

		class Bundle
		{
			friend class BundleStreamWriter;

		public:
			enum FLAGS
			{
				FRAGMENT = 1 << 0x00,
				APPDATA_IS_ADMRECORD = 1 << 0x01,
				DONT_FRAGMENT = 1 << 0x02,
				CUSTODY_REQUESTED = 1 << 0x03,
				DESTINATION_IS_SINGLETON = 1 << 0x04,
				ACKOFAPP_REQUESTED = 1 << 0x05,
				RESERVED_6 = 1 << 0x06,
				PRIORITY_BIT1 = 1 << 0x07,
				PRIORITY_BIT2 = 1 << 0x08,
				CLASSOFSERVICE_9 = 1 << 0x09,
				CLASSOFSERVICE_10 = 1 << 0x0A,
				CLASSOFSERVICE_11 = 1 << 0x0B,
				CLASSOFSERVICE_12 = 1 << 0x0C,
				CLASSOFSERVICE_13 = 1 << 0x0D,
				REQUEST_REPORT_OF_BUNDLE_RECEPTION = 1 << 0x0E,
				REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE = 1 << 0x0F,
				REQUEST_REPORT_OF_BUNDLE_FORWARDING = 1 << 0x10,
				REQUEST_REPORT_OF_BUNDLE_DELIVERY = 1 << 0x11,
				REQUEST_REPORT_OF_BUNDLE_DELETION = 1 << 0x12,
				STATUS_REPORT_REQUEST_19 = 1 << 0x13,
				STATUS_REPORT_REQUEST_20 = 1 << 0x14
			};

			Bundle();
			virtual ~Bundle();

			bool operator==(const Bundle& other) const;
			bool operator!=(const Bundle& other) const;
			bool operator<(const Bundle& other) const;
			bool operator>(const Bundle& other) const;

			void addBlock(Block *b);

			const list<Block*> getBlocks(char type) const;
			const list<Block*> getBlocks() const;

			template<typename T>
			const std::list<T> getBlocks() const;

			void removeBlock(Block *b);
			void clearBlocks();

			bool isExpired() const;
			string toString() const;

			size_t getSize() const;

			/**
			 * relabel the bundle with a new sequence number and a timestamp
			 */
			void relabel();

		private:
			static size_t __sequencenumber;
			std::list< refcnt_ptr<Block> > _blocks;

			friend std::ostream &operator<<(std::ostream &stream, const dtn::data::Bundle &obj);
			friend std::istream &operator>>(std::istream &stream, dtn::data::Bundle &b);

		public:
			size_t _procflags;
			size_t _timestamp;
			size_t _sequencenumber;
			size_t _lifetime;
			size_t _fragmentoffset;
			size_t _appdatalength;

			EID _source;
			EID _destination;
			EID _reportto;
			EID _custodian;
		};

		template<typename T>
		const std::list<T> Bundle::getBlocks() const
		{
			// create a list of blocks
			list<T> ret;

			// copy all blocks to the list
			for (list< refcnt_ptr<Block> >::const_iterator iter = _blocks.begin(); iter != _blocks.end(); iter++)
			{
				refcnt_ptr<Block> blockref = (*iter);

				if (blockref->_blocktype == T::BLOCK_TYPE)
				{
					ret.push_back( T(blockref.getPointer()) );
				}
			}

			return ret;
		}
	}
}

#endif /* BUNDLE_H_ */
