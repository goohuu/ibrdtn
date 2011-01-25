/*
 * EpidemicControlMessage.h
 *
 *  Created on: 24.01.2011
 *      Author: morgenro
 */

#ifndef EPIDEMICCONTROLMESSAGE_H_
#define EPIDEMICCONTROLMESSAGE_H_

#include "routing/SummaryVector.h"
#include <ibrdtn/data/SDNV.h>
#include <ibrdtn/data/BundleString.h>
#include <iostream>

namespace dtn
{
	namespace routing
	{
		class EpidemicControlMessage
		{
		public:
			enum MessageType
			{
				ECM_QUERY_SUMMARY_VECTOR = 0,
				ECM_RESPONSE = 1
			} type;

			enum MessageFlags
			{
				ECM_CONTAINS_SUMMARY_VECTOR = 1 << 0,
				ECM_CONTAINS_PURGE_VECTOR = 1 << 1
			};

			char flags;

			EpidemicControlMessage();
			virtual ~EpidemicControlMessage();

			void setPurgeVector(const SummaryVector &vector);
			void setSummaryVector(const SummaryVector &vector);
			const SummaryVector& getSummaryVector() const;
			const SummaryVector& getPurgeVector() const;

			friend std::ostream& operator<<(std::ostream&, const EpidemicControlMessage&);
			friend std::istream& operator>>(std::istream&, EpidemicControlMessage&);

		private:
			dtn::data::SDNV _counter;
			dtn::data::BundleString _data;
			SummaryVector _vector;
			SummaryVector _purge;
		};
	}
}

#endif /* EPIDEMICCONTROLMESSAGE_H_ */
