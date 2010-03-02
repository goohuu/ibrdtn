/*
 * SummaryVector.cpp
 *
 *  Created on: 02.03.2010
 *      Author: morgenro
 */

#include "routing/SummaryVector.h"

namespace dtn
{
	namespace routing
	{
		SummaryVector::SummaryVector(const std::set<dtn::routing::MetaBundle> &list)
		 : data("Not implemented!")
		{
			add(list);
		}

		SummaryVector::SummaryVector()
		 : data("Not implemented!")
		{
		}

		SummaryVector::~SummaryVector()
		{
		}

		void SummaryVector::add(const std::set<dtn::routing::MetaBundle> &list)
		{
		}

		bool SummaryVector::contains(const dtn::data::BundleID &id) const
		{
		}

		void SummaryVector::add(const dtn::data::BundleID &id)
		{
		}

		void SummaryVector::clear()
		{
		}

		std::ostream &operator<<(std::ostream &stream, const SummaryVector &obj)
		{
			stream << obj.data;
		}

		std::istream &operator>>(std::istream &stream, SummaryVector &obj)
		{
			stream >> obj.data;
		}
	}
}
