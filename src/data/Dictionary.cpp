/*
 * Dictionary.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/Dictionary.h"
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

	Dictionary::Dictionary(const char *data, size_t size)
	{
		_bytestream.write(data, size);
	}

	void Dictionary::read(const char *data, size_t size)
	{
		_bytestream.str("");
		_bytestream.write(data, size);
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

	size_t Dictionary::writeRef( BundleWriter &writer, const EID &eid ) const
	{
		return writer.write( getRef(eid) );
	}

	size_t Dictionary::writeLength( BundleWriter &writer ) const
	{
		return writer.write( _bytestream.str().length() );
	}

	size_t Dictionary::writeByteArray( BundleWriter &writer ) const
	{
		return writer.write( (istream&)_bytestream );
	}
}
}
