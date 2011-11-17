/*
 * StreamBlock.h
 *
 *  Created on: 17.11.2011
 *      Author: morgenro
 */

#ifndef STREAMBLOCK_H_
#define STREAMBLOCK_H_

#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/ExtensionBlock.h"

namespace dtn
{
	namespace data
	{
		class StreamBlock : public dtn::data::Block
		{
		public:
			class Factory : public dtn::data::ExtensionBlock::Factory
			{
			public:
				Factory() : dtn::data::ExtensionBlock::Factory(StreamBlock::BLOCK_TYPE) {};
				virtual ~Factory() {};
				virtual dtn::data::Block* create();
			};

			static const char BLOCK_TYPE = 242;

			StreamBlock();
			virtual ~StreamBlock();

			virtual size_t getLength() const;
			virtual std::ostream &serialize(std::ostream &stream, size_t &length) const;
			virtual std::istream &deserialize(std::istream &stream, const size_t length);

			void setSequenceNumber(size_t seq);
			size_t getSequenceNumber() const;

		private:
			dtn::data::SDNV _seq;
		};

		/**
		 * This creates a static block factory
		 */
		static StreamBlock::Factory __StreamBlockFactory__;
	} /* namespace data */
} /* namespace dtn */
#endif /* STREAMBLOCK_H_ */
