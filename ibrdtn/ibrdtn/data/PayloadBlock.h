/*
 * PayloadBlock.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef PAYLOADBLOCK_H_
#define PAYLOADBLOCK_H_

#include "ibrdtn/data/Block.h"
#include "ibrcommon/data/BLOB.h"

namespace dtn
{
	namespace data
	{
		class PayloadBlock : public Block
		{
		public:
			static const char BLOCK_TYPE = 1;

			PayloadBlock();
			PayloadBlock(ibrcommon::BLOB::Reference ref);
			virtual ~PayloadBlock();

			ibrcommon::BLOB::Reference getBLOB() const;

			virtual size_t getLength() const;
			virtual std::ostream &serialize(std::ostream &stream) const;
			virtual std::istream &deserialize(std::istream &stream);

		private:
			ibrcommon::BLOB::Reference _blobref;
		};
	}
}

#endif /* PAYLOADBLOCK_H_ */
