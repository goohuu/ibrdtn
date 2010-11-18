/*
 * Block.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#ifndef BLOCK_H_
#define BLOCK_H_


#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/Dictionary.h"
#include "ibrdtn/data/Serializer.h"
#include <ibrcommon/Exceptions.h>

#include <string>

namespace dtn
{
	namespace data
	{
		class Bundle;

		class Block
		{
			friend class Bundle;
			friend class DefaultSerializer;
			friend class DefaultDeserializer;
			friend class SeparateSerializer;
			friend class SeparateDeserializer;

		public:
			enum ProcFlags
			{
				REPLICATE_IN_EVERY_FRAGMENT = 1, 			// 0 - Block must be replicated in every fragment.
				TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED = 1 << 0x01, // 1 - Transmit status report if block can't be processed.
				DELETE_BUNDLE_IF_NOT_PROCESSED = 1 << 0x02, 		// 2 - Delete bundle if block can't be processed.
				LAST_BLOCK = 1 << 0x03,								// 3 - Last block.
				DISCARD_IF_NOT_PROCESSED = 1 << 0x04,				// 4 - Discard block if it can't be processed.
				FORWARDED_WITHOUT_PROCESSED = 1 << 0x05,			// 5 - Block was forwarded without being processed.
				BLOCK_CONTAINS_EIDS = 1 << 0x06						// 6 - Block contains an EID-reference field.
			};

			virtual ~Block();

			virtual void addEID(const dtn::data::EID &eid);
			virtual std::list<dtn::data::EID> getEIDList() const;

			char getType() const { return _blocktype; }

			void set(ProcFlags flag, const bool &value);
			bool get(ProcFlags flag) const;

		protected:
			/**
			 * The constructor of this class is protected to prevent instantiation of this abstract class.
			 * @param bundle
			 * @param blocktype
			 * @return
			 */
			Block(char blocktype);
			char _blocktype;
			size_t _blocksize;
			std::list<dtn::data::EID> _eids;

			virtual size_t getLength() const = 0;
			virtual std::ostream &serialize(std::ostream &stream) const = 0;
			virtual std::istream &deserialize(std::istream &stream) = 0;

		private:
			size_t _procflags;
		};
	}
}

#endif /* BLOCK_H_ */
