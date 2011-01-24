/*
 * EpidemicExtensionBlock.cpp
 *
 *  Created on: 24.01.2011
 *      Author: morgenro
 */

#include "routing/epidemic/EpidemicExtensionBlock.h"

namespace dtn
{
	namespace routing
	{
		dtn::data::Block* EpidemicExtensionBlock::Factory::create()
		{
			return new EpidemicExtensionBlock();
		}

		EpidemicExtensionBlock::EpidemicExtensionBlock()
		 : dtn::data::Block(EpidemicExtensionBlock::BLOCK_TYPE), _data("forwarded through epidemic routing")
		{
		}

		EpidemicExtensionBlock::~EpidemicExtensionBlock()
		{
		}

		void EpidemicExtensionBlock::setSummaryVector(const SummaryVector &vector)
		{
			_vector = vector;
		}

		const SummaryVector& EpidemicExtensionBlock::getSummaryVector() const
		{
			return _vector;
		}

		void EpidemicExtensionBlock::setPurgeVector(const SummaryVector &vector)
		{
			_purge = vector;
		}

		const SummaryVector& EpidemicExtensionBlock::getPurgeVector() const
		{
			return _purge;
		}

		void EpidemicExtensionBlock::set(dtn::data::SDNV value)
		{
			_counter = value;
		}

		dtn::data::SDNV EpidemicExtensionBlock::get() const
		{
			return _counter;
		}

		size_t EpidemicExtensionBlock::getLength() const
		{
			return _counter.getLength() + _data.getLength() + _vector.getLength();
		}

		std::istream& EpidemicExtensionBlock::deserialize(std::istream &stream)
		{
			stream >> _counter;
			stream >> _data;
			stream >> _vector;
			stream >> _purge;

			// unset block not processed bit
			dtn::data::Block::set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, false);

			return stream;
		}

		std::ostream &EpidemicExtensionBlock::serialize(std::ostream &stream) const
		{
			stream << _counter;
			stream << _data;
			stream << _vector;
			stream << _purge;

			return stream;
		}

		/**
		 * This creates a static bundle factory
		 */
		static EpidemicExtensionBlock::Factory __EpidemicExtensionFactory__;
	}
}
