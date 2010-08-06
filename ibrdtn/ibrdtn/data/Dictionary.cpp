/*
 * Dictionary.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Dictionary.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/Exceptions.h"
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
	Dictionary& Dictionary::operator=(const Dictionary &d)
	{
		_bytestream.str("");
		_bytestream << d._bytestream.rdbuf();
		return (*this);
	}

	Dictionary::~Dictionary()
	{
	}

	size_t Dictionary::get(const std::string value) const
	{
		std::string bytes = _bytestream.str();
		const char *bytebegin = bytes.c_str();
		const char *bytepos = bytebegin;
		const char *byteend = bytebegin + bytes.length() + 1;

		if (bytes.length() <= 0) return std::string::npos;

		while (bytepos < byteend)
		{
			std::string dictstr(bytepos);

			if (dictstr == value)
			{
				return bytepos - bytebegin;
			}

			bytepos += dictstr.length() + 1;
		}

		return std::string::npos;
	}

	bool Dictionary::exists(const std::string value) const
	{
		std::string bytes = _bytestream.str();
		const char *bytepos = bytes.c_str();
		const char *byteend = bytepos + bytes.length();

		if (bytes.length() <= 0) return false;

		while (bytepos < byteend)
		{
			std::string dictstr(bytepos);

			if (dictstr == value)
			{
				return true;
			}

			bytepos += dictstr.length() + 1;
		}

		return false;
	}

	void Dictionary::add(const std::string value)
	{
		if (!exists(value))
		{
			_bytestream << value << '\0';
		}
	}

	void Dictionary::add(const EID &eid)
	{
		add(eid.getScheme());
		add(eid.getNode() + eid.getApplication());
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
		_bytestream.str("");
	}

	size_t Dictionary::getSize() const
	{
		return _bytestream.str().length();
	}

	pair<size_t, size_t> Dictionary::getRef(const EID &eid) const
	{
		const string scheme = eid.getScheme();
		const string ssp = eid.getNode() + eid.getApplication();
		return make_pair(get(scheme), get(ssp));
	}

	std::ostream &operator<<(std::ostream &stream, const dtn::data::Dictionary &obj)
	{
		dtn::data::SDNV length(obj.getSize());
		stream << length;
		stream << obj._bytestream.rdbuf();

		return stream;
	}

	std::istream &operator>>(std::istream &stream, dtn::data::Dictionary &obj)
	{
		dtn::data::SDNV length;
		stream >> length;

		// if the dictionary size if zero throw a exception
		if (length.getValue() <= 0)
			throw dtn::InvalidDataException("Dictionary size is zero!");

		obj._bytestream.str("");
		char data[length.getValue()];
		stream.read(data, length.getValue());
		obj._bytestream.write(data, length.getValue());

		return stream;
	}
}
}
