/*
 * Dictionary.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Dictionary.h"
#include "ibrdtn/data/SDNV.h"
#include <map>
#include <stdexcept>
#include <string.h>
#include <iostream>

using namespace std;

namespace dtn
{
namespace data
{
	Dictionary::Dictionary()
	{
	}

	Dictionary::Dictionary(const Dictionary &d)
	{
		_bytestream << d._bytestream.rdbuf();
	}

	/**
	 * assign operator
	 */
	Dictionary Dictionary::operator=(const Dictionary &d)
	{
		_bytestream.clear();
		_bytestream << d._bytestream.rdbuf();
		return (*this);
	}

	Dictionary::~Dictionary()
	{
	}

	void Dictionary::add(const EID &eid)
	{
		string scheme = eid.getScheme();
		string ssp = eid.getNode() + eid.getApplication();

		string bytearray = _bytestream.str();

		if ( bytearray.find(scheme) == string::npos )
		{
			_bytestream << scheme << '\0';
		}

		if ( bytearray.find(ssp) == string::npos )
		{
			_bytestream << ssp << '\0';
		}
	}

	void Dictionary::add(const list<EID> &eids)
	{
		list<EID>::const_iterator iter = eids.begin();

		while (iter != eids.end())
		{
			add(*iter);
			iter++;
		}
	}

	EID Dictionary::get(size_t scheme, size_t ssp)
	{
		char buffer[1024];

		_bytestream.seekg(scheme);
		_bytestream.get(buffer, 1024, '\0');
		string scheme_str(buffer);

		_bytestream.seekg(ssp);
		_bytestream.get(buffer, 1024, '\0');
		string ssp_str(buffer);

		return EID(scheme_str, ssp_str);
	}

	void Dictionary::clear()
	{
		_bytestream.clear();
	}

	size_t Dictionary::getSize() const
	{
		return _bytestream.str().length();
	}

	pair<size_t, size_t> Dictionary::getRef(const EID &eid) const
	{
		string scheme = eid.getScheme();
		string ssp = eid.getNode() + eid.getApplication();

		string bytearray = _bytestream.str();
		size_t scheme_pos = bytearray.find(scheme);
		size_t ssp_pos = bytearray.find(ssp);

		return make_pair(scheme_pos, ssp_pos);
	}

	std::ostream &operator<<(std::ostream &stream, const dtn::data::Dictionary &obj)
	{
		dtn::data::SDNV length(obj.getSize());
		stream << length;
		stream << obj._bytestream.rdbuf();
	}

	std::istream &operator>>(std::istream &stream, dtn::data::Dictionary &obj)
	{
		dtn::data::SDNV length;
		stream >> length;

		obj._bytestream.str("");
		char data[length.getValue()];
		stream.read(data, length.getValue());
		obj._bytestream.write(data, length.getValue());
	}
}
}
