/*
 * EID.cpp
 *
 *  Created on: 09.03.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/EID.h"

namespace dtn
{
namespace data
{
	EID::EID()
	: _scheme("dtn"), _ssp("none")
	{
	}

	EID::EID(const EID &other)
	 : _scheme(other._scheme), _ssp(other._ssp)
	{
	}

	EID::EID(std::string scheme, std::string ssp)
	 : _scheme(scheme), _ssp(ssp)
	{
		// TODO: checks for illegal characters
	}

	EID::EID(std::string value)
	{
		try {
			// search for the delimiter
			size_t delimiter = value.find_first_of(":");

			// jump to default eid if the format is wrong
			if (delimiter == std::string::npos)
				throw ibrcommon::Exception("wrong eid format");

			// the scheme is everything before the delimiter
			_scheme = value.substr(0, delimiter);

			// the ssp is everything else
			size_t startofssp = delimiter + 1;
			_ssp = value.substr(startofssp, value.length() - startofssp);

			// TODO: do syntax check
		} catch (...) {
			_scheme = "dtn";
			_ssp = "none";
		}
	}

	EID::~EID()
	{
	}

	EID& EID::operator=(const EID &other)
	{
		_ssp = other._ssp;
		_scheme = other._scheme;
		return *this;
	}

	bool EID::operator==(EID const& other) const
	{
		return (_ssp == other._ssp) && (_scheme == other._scheme);
	}

	bool EID::operator==(string const& other) const
	{
		return ((*this) == EID(other));
	}

	bool EID::operator!=(EID const& other) const
	{
		return !((*this) == other);
	}

	EID EID::operator+(string suffix)
	{
		return EID(getString() + suffix);
	}

	bool EID::sameHost(string const& other) const
	{
		return ( other == getNodeEID() );
	}

	bool EID::sameHost(EID const& other) const
	{
		return ( other.getNodeEID() == getNodeEID() );
	}

	bool EID::operator<(EID const& other) const
	{
		return getString() < other.getString();
	}

	bool EID::operator>(EID const& other) const
	{
		return other < (*this);
	}

	std::string EID::getString() const
	{
		return _scheme + ":" + _ssp;
	}

	std::string EID::getApplication() const throw (ibrcommon::Exception)
	{
		// first char not "/", e.g. "//node1" -> 2
		size_t first_char = _ssp.find_first_not_of("/");

		// only "/" ? thats bad!
		if (first_char == std::string::npos)
			throw ibrcommon::Exception("wrong eid format");

		// start of application part
		size_t application_start = _ssp.find_first_of("/", first_char);

		// no application part available
		if (application_start == std::string::npos)
			return "";

		// return the application part
		return _ssp.substr(application_start, _ssp.length() - application_start);
	}

	std::string EID::getNode() const throw (ibrcommon::Exception)
	{
		// first char not "/", e.g. "//node1" -> 2
		size_t first_char = _ssp.find_first_not_of("/");

		// only "/" ? thats bad!
		if (first_char == std::string::npos)
			throw ibrcommon::Exception("wrong eid format");

		// start of application part
		size_t application_start = _ssp.find_first_of("/", first_char);

		// no application part available
		if (application_start == std::string::npos)
			return _ssp;

		// return the node part
		return _ssp.substr(0, application_start);
	}

	std::string EID::getScheme() const
	{
		return _scheme;
	}

	std::string EID::getNodeEID() const
	{
		return _scheme + ":" + getNode();
	}

	bool EID::hasApplication() const
	{
		// first char not "/", e.g. "//node1" -> 2
		size_t first_char = _ssp.find_first_not_of("/");

		// only "/" ? thats bad!
		if (first_char == std::string::npos)
			throw ibrcommon::Exception("wrong eid format");

		// start of application part
		size_t application_start = _ssp.find_first_of("/", first_char);

		// no application part available
		if (application_start == std::string::npos)
			return false;

		// return the application part
		return true;
	}
}
}
