/*
 * SummaryVector.h
 *
 *  Created on: 02.03.2010
 *      Author: morgenro
 */

#ifndef SUMMARYVECTOR_H_
#define SUMMARYVECTOR_H_

#include "ibrdtn/data/BundleID.h"
#include "routing/MetaBundle.h"
#include "ibrdtn/data/BundleString.h"
#include <iostream>
#include <set>

namespace dtn
{
	namespace routing
	{
		class SummaryVector
		{
		public:
			SummaryVector(const std::set<dtn::routing::MetaBundle> &list);
			SummaryVector();
			virtual ~SummaryVector();

			virtual bool contains(const dtn::data::BundleID &id) const;
			virtual void add(const dtn::data::BundleID &id);
			virtual void clear();

			virtual void add(const std::set<dtn::routing::MetaBundle> &list);

			friend std::ostream &operator<<(std::ostream &stream, const SummaryVector &obj);
			friend std::istream &operator>>(std::istream &stream, SummaryVector &obj);

		private:
			dtn::data::BundleString data;
		};
	}
}


#endif /* SUMMARYVECTOR_H_ */
