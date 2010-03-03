/*
 * BundleID.cpp
 *
 *  Created on: 01.09.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/BundleID.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BundleString.h"

namespace dtn
{
	namespace data
	{
		BundleID::BundleID()
		: _source(), _timestamp(0), _sequencenumber(0), _fragment(false), _offset(0)
		{
		}

		BundleID::BundleID(EID source, size_t timestamp, size_t sequencenumber, bool fragment, size_t offset)
		: _source(source), _timestamp(timestamp), _sequencenumber(sequencenumber), _fragment(fragment), _offset(offset)
		{
		}

		BundleID::BundleID(const dtn::data::Bundle &b)
		: _source(b._source), _timestamp(b._timestamp), _sequencenumber(b._sequencenumber),
		_fragment(b._procflags & dtn::data::Bundle::FRAGMENT), _offset(b._fragmentoffset)
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

			if (_fragment)
			{
				if (other._offset < _offset) return true;
			}

			return false;
		}

		bool BundleID::operator>(const BundleID& other) const
		{
			if (other._source > _source) return true;
			if (other._timestamp > _timestamp) return true;
			if (other._sequencenumber > _sequencenumber) return true;

			if (_fragment)
			{
				if (other._offset > _offset) return true;
			}

			return false;
		}

		bool BundleID::operator!=(const BundleID& other) const
		{
			return !((*this) == other);
		}

		bool BundleID::operator==(const BundleID& other) const
		{
			if (other._timestamp != _timestamp) return false;
			if (other._sequencenumber != _sequencenumber) return false;
			if (other._source != _source) return false;

			if (_fragment)
			{
				if (other._offset != _offset) return false;
			}

			return true;
		}

		size_t BundleID::getTimestamp() const
		{
			return _timestamp;
		}

		string BundleID::toString() const
		{
			stringstream ss;
			ss << "[" << _timestamp << "." << _sequencenumber;

			if (_fragment)
			{
				ss << "." << _offset;
			}

			ss << "] " << _source.getString();

			return ss.str();
		}

		std::ostream &operator<<(std::ostream &stream, const BundleID &obj)
		{
			dtn::data::SDNV timestamp(obj._timestamp);
			dtn::data::SDNV sequencenumber(obj._sequencenumber);
			dtn::data::SDNV offset(obj._offset);
			dtn::data::BundleString source(obj._source.getString());

			stream << timestamp << sequencenumber << offset << source;
		}

		std::istream &operator>>(std::istream &stream, BundleID &obj)
		{
			dtn::data::SDNV timestamp;
			dtn::data::SDNV sequencenumber;
			dtn::data::SDNV offset;
			dtn::data::BundleString source;

			stream >> timestamp >> sequencenumber >> offset >> source;

			obj._timestamp = timestamp.getValue();
			obj._sequencenumber = sequencenumber.getValue();
			obj._offset = offset.getValue();
			obj._source = dtn::data::EID(source);
		}
	}
}
