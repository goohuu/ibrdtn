/*
 * BundleID.cpp
 *
 *  Created on: 01.09.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/BundleID.h"


namespace dtn
{
	namespace data
	{
		BundleID::BundleID(EID source, size_t timestamp, size_t sequencenumber)
		: _source(source), _timestamp(timestamp), _sequencenumber(sequencenumber)
		{
		}

		BundleID::BundleID(const dtn::data::Bundle &b)
		: _source(b._source), _timestamp(b._timestamp), _sequencenumber(b._sequencenumber)
		{
		}

		BundleID::~BundleID()
		{
		}

		bool BundleID::operator<(const BundleID& other) const
		{
			if (other._source < _source) return true;
			if (other._timestamp < _timestamp) return true;
			if (other._sequencenumber < _sequencenumber) return true;
			return false;
		}

		bool BundleID::operator>(const BundleID& other) const
		{
			if (other._source > _source) return true;
			if (other._timestamp > _timestamp) return true;
			if (other._sequencenumber > _sequencenumber) return true;
			return false;
		}

		bool BundleID::operator==(const BundleID& other) const
		{
			if (other._timestamp != _timestamp) return false;
			if (other._sequencenumber != _sequencenumber) return false;
			if (other._source != _source) return false;
			return true;
		}

		string BundleID::toString() const
		{
			stringstream ss;
			ss << "[" << _timestamp << "." << _sequencenumber << "] " << _source.getString();
			return ss.str();
		}
	}
}
