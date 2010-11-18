/*
 * AgeBlock.cpp
 *
 *  Created on: 18.11.2010
 *      Author: morgenro
 */

#include "AgeBlock.h"

namespace dtn
{
	namespace data
	{
		dtn::data::Block* AgeBlock::Factory::create()
		{
			return new AgeBlock::AgeBlock();
		}

		AgeBlock::AgeBlock()
		 : dtn::data::Block(AgeBlock::BLOCK_TYPE)
		{
			_time.start();
		}

		AgeBlock::~AgeBlock()
		{
		}

		size_t AgeBlock::getAge() const
		{
			ibrcommon::TimeMeasurement time = this->_time;
			time.stop();
			return _age.getValue() + time.getSeconds();
		}

		size_t AgeBlock::getLength() const
		{
			return _age.getLength();
		}

		std::ostream& AgeBlock::serialize(std::ostream &stream) const
		{
			stream << SDNV(getAge());
			return stream;
		}

		std::istream& AgeBlock::deserialize(std::istream &stream)
		{
			stream >> _age;
			_time.start();
			return stream;
		}
	}
}
