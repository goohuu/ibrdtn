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
			_ids.insert(list.begin(), list.end());
		}

		bool SummaryVector::contains(const dtn::data::BundleID &id) const
		{
			return (_ids.find(id) != _ids.end());
		}

		void SummaryVector::add(const dtn::data::BundleID &id)
		{
			_ids.insert(id);
		}

		void SummaryVector::clear()
		{
			_ids.clear();
		}

		size_t SummaryVector::count() const
		{
			return _ids.size();
		}

		std::ostream &operator<<(std::ostream &stream, const SummaryVector &obj)
		{
			dtn::data::SDNV count(obj._ids.size());

			stream << count;

			for (std::set<dtn::data::BundleID>::iterator iter = obj._ids.begin(); iter != obj._ids.end(); iter++)
			{
				stream << (*iter);
			}
		}

		std::istream &operator>>(std::istream &stream, SummaryVector &obj)
		{
			dtn::data::SDNV count;
			stream >> count;

			obj._ids.clear();

			for (size_t i = 0; i < count.getValue(); i++)
			{
				dtn::data::BundleID id;
				stream >> id;
				obj._ids.insert(id);
			}
		}
	}
}
