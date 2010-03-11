/*
 * PayloadBlock.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef PAYLOADBLOCK_H_
#define PAYLOADBLOCK_H_

#include "ibrdtn/streams/BundleWriter.h"
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
			PayloadBlock(Block *block);
			PayloadBlock(ibrcommon::BLOB::Reference ref);
			virtual ~PayloadBlock();

			//std::pair<PayloadBlock*, PayloadBlock*> split(size_t position);

			virtual void read() {};
			virtual void commit() {};

			/**
			 * This method accepts a offset of bytes which are already successful transferred to
			 * another Node.
			 * @param offset
			 */
			void setFragment(size_t offset) throw (dtn::exceptions::FragmentationException);
		};
	}
}

#endif /* PAYLOADBLOCK_H_ */
