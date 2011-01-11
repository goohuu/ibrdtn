/*
 * AgeBlock.cpp
 *
 *  Created on: 18.11.2010
 *      Author: morgenro
 */

#include "ibrdtn/data/AgeBlock.h"

namespace dtn
{
	namespace data
	{
		dtn::data::Block* AgeBlock::Factory::create()
		{
			return new AgeBlock();
		}

		AgeBlock::AgeBlock()
		 : dtn::data::Block(AgeBlock::BLOCK_TYPE)
		{
			_time.start();

			// set the replicate in every fragment bit
			set(REPLICATE_IN_EVERY_FRAGMENT, true);
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

		void AgeBlock::addAge(size_t value)
		{
			_age += value;
		}

		void AgeBlock::setAge(size_t value)
		{
			_age = value;
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

		size_t AgeBlock::getLength_mutable() const
		{
			return 1;
		}

		std::ostream& AgeBlock::serialize_mutable(std::ostream &stream) const
		{
			// we have to ignore the age field, because this is very dynamic data
			stream << (char)0;
			return stream;
		}

		size_t AgeBlock::getLength_strict() const
		{
			return 1;
		}

		std::ostream& AgeBlock::serialize_strict(std::ostream &stream) const
		{
			// we have to ignore the age field, because this is very dynamic data
			stream << (char)0;
			return stream;
		}
	}
}
