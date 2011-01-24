/*
 * EpidemicExtensionBlock.h
 *
 *  Created on: 24.01.2011
 *      Author: morgenro
 */

#ifndef EPIDEMICEXTENSIONBLOCK_H_
#define EPIDEMICEXTENSIONBLOCK_H_

#include "routing/SummaryVector.h"
#include <ibrdtn/data/Block.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <ibrdtn/data/ExtensionBlock.h>

namespace dtn
{
	namespace routing
	{
		class EpidemicExtensionBlock : public dtn::data::Block
		{
		public:
			class Factory : public dtn::data::ExtensionBlock::Factory
			{
			public:
				Factory() : dtn::data::ExtensionBlock::Factory(EpidemicExtensionBlock::BLOCK_TYPE) {};
				virtual ~Factory() {};
				virtual dtn::data::Block* create();
			};

			static const char BLOCK_TYPE = 201;

			EpidemicExtensionBlock();
			virtual ~EpidemicExtensionBlock();

			void set(dtn::data::SDNV value);
			dtn::data::SDNV get() const;

			void setPurgeVector(const SummaryVector &vector);
			void setSummaryVector(const SummaryVector &vector);
			const SummaryVector& getSummaryVector() const;
			const SummaryVector& getPurgeVector() const;

			virtual size_t getLength() const;
			virtual std::ostream &serialize(std::ostream &stream) const;
			virtual std::istream &deserialize(std::istream &stream);

		private:
			dtn::data::SDNV _counter;
			dtn::data::BundleString _data;
			SummaryVector _vector;
			SummaryVector _purge;
		};
	}
}

#endif /* EPIDEMICEXTENSIONBLOCK_H_ */
