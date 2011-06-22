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

		size_t AgeBlock::getMicroseconds() const
		{
			ibrcommon::TimeMeasurement time = this->_time;
			time.stop();

			return _age.getValue() + time.getMicroseconds();
		}

		size_t AgeBlock::getSeconds() const
		{
			ibrcommon::TimeMeasurement time = this->_time;
			time.stop();

			return (_age.getValue() + time.getMicroseconds()) / 1000000;
		}

		void AgeBlock::addSeconds(size_t value)
		{
			_age += (value * 1000000);
		}

		void AgeBlock::setSeconds(size_t value)
		{
			_age = value * 1000000;
		}

		size_t AgeBlock::getLength() const
		{
			return SDNV(getMicroseconds()).getLength();
		}

		std::ostream& AgeBlock::serialize(std::ostream &stream, size_t &length) const
		{
			SDNV value(getMicroseconds());
			stream << value;
			length += value.getLength();
			return stream;
		}

		std::istream& AgeBlock::deserialize(std::istream &stream, const size_t length)
		{
			stream >> _age;
			_time.start();
			return stream;
		}

		size_t AgeBlock::getLength_strict() const
		{
			return 1;
		}

		std::ostream& AgeBlock::serialize_strict(std::ostream &stream, size_t &length) const
		{
			// we have to ignore the age field, because this is very dynamic data
			stream << (char)0;
			length += 1;
			return stream;
		}
	}
}
