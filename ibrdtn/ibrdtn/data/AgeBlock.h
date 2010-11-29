/*
 * AgeBlock.h
 *
 *  Created on: 18.11.2010
 *      Author: morgenro
 */

#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/ExtensionBlock.h>
#include <ibrcommon/TimeMeasurement.h>

#ifndef AGEBLOCK_H_
#define AGEBLOCK_H_

namespace dtn
{
	namespace data
	{
		class AgeBlock : public dtn::data::Block
		{
		public:
			class Factory : public dtn::data::ExtensionBlock::Factory
			{
			public:
				Factory() : dtn::data::ExtensionBlock::Factory(AgeBlock::BLOCK_TYPE) {};
				virtual ~Factory() {};
				virtual dtn::data::Block* create();
			};

			static const char BLOCK_TYPE = 10;

			AgeBlock();
			virtual ~AgeBlock();

			virtual size_t getLength() const;
			virtual std::ostream &serialize(std::ostream &stream) const;
			virtual std::istream &deserialize(std::istream &stream);

			size_t getAge() const;

			/**
			 * set the age
			 */
			void setAge(size_t value);

			/**
			 * add a value to the age
			 */
			void addAge(size_t value);

		private:
			dtn::data::SDNV _age;
			ibrcommon::TimeMeasurement _time;
		};

		/**
		 * This creates a static block factory
		 */
		static AgeBlock::AgeBlock::Factory __AgeBlockFactory__;
	}
}

#endif /* AGEBLOCK_H_ */
