/*
 * Block.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#ifndef BLOCK_H_
#define BLOCK_H_

#include "ibrdtn/default.h"
#include "ibrdtn/streams/BundleWriter.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/data/EID.h"
#include "ibrcommon/data/BLOB.h"
#include "ibrcommon/Exceptions.h"
#include "ibrdtn/data/Exceptions.h"

#include <string>

namespace dtn
{
	namespace data
	{
		class Bundle;

		class Block
		{
			friend class Bundle;
			friend class dtn::streams::BundleStreamWriter;

		public:
			enum ProcFlags
			{
				REPLICATE_IN_EVERY_FRAGMENT = 1 << 0x01, 			// 0 - Block must be replicated in every fragment.
				TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED = 1 << 0x02, // 1 - Transmit status report if block can't be processed.
				DELETE_BUNDLE_IF_NOT_PROCESSED = 1 << 0x03, 		// 2 - Delete bundle if block can't be processed.
				LAST_BLOCK = 1 << 0x04,								// 3 - Last block.
				DISCARD_IF_NOT_PROCESSED = 1 << 0x05,				// 4 - Discard block if it can't be processed.
				FORWARDED_WITHOUT_PROCESSED = 1 << 0x06,			// 5 - Block was forwarded without being processed.
				BLOCK_CONTAINS_EIDS = 1 << 0x07						// 6 - Block contains an EID-reference field.
			};

			Block(char blocktype);
			Block(char blocktype, ibrcommon::BLOB::Reference ref);
			virtual ~Block();

			virtual void addEID(EID eid);
			virtual list<EID> getEIDList() const;

			size_t getSize() const;

			ibrcommon::BLOB::Reference getBLOB();

			size_t _procflags;

			virtual void read() {}
			virtual void commit() {};

			char getType() { return _blocktype; }

		protected:
			class PayloadTooSmallException : public ibrcommon::Exception
			{
			public:
				PayloadTooSmallException(std::string what = "The payload is too small for this action.") throw() : ibrcommon::Exception(what)
				{
				};
			};

			Block(Block *block);

			size_t writeHeader( dtn::streams::BundleWriter &writer ) const;
			size_t getHeaderSize(dtn::streams::BundleWriter &writer) const;

			/**
			 * This method sets an offset for this block. If the block gets serialized the
			 * first <offset> bytes are cut off. A PayloadTooSmallException is thrown, if the payload is
			 * smaller than the offset.
			 * @param offset The offset for this block.
			 */
			void setOffset(size_t offset) throw (PayloadTooSmallException);

			char _blocktype;
			list<EID> _eids;
			ibrcommon::BLOB::Reference _blobref;

			virtual size_t write( dtn::streams::BundleWriter &writer );

		private:
			size_t _payload_offset;
		};
	}
}

#endif /* BLOCK_H_ */
