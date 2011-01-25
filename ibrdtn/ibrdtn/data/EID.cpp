/*
 * EID.cpp
 *
 *  Created on: 09.03.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/EID.h"
#include <sstream>
#include <iostream>

namespace dtn
{
	namespace data
	{
		const std::string EID::DEFAULT_SCHEME = "dtn";
		const std::string EID::CBHE_SCHEME = "ipn";

		EID::EID()
		: _scheme(DEFAULT_SCHEME), _ssp("none")
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
				_scheme = DEFAULT_SCHEME;
				_ssp = "none";
			}
		}

		EID::EID(size_t node, size_t application)
		 : _scheme(EID::CBHE_SCHEME), _ssp()
		{
			if (node == 0)
			{
				_scheme = DEFAULT_SCHEME;
				_ssp = "none";
				return;
			}

			std::stringstream ss_ssp;
			ss_ssp << node << "." << application;
			_ssp = ss_ssp.str();
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

		EID EID::operator+(string suffix) const
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
			size_t first_char = 0;
			char delimiter = '.';

			if (_scheme != EID::CBHE_SCHEME)
			{
				// with a uncompressed bundle header we have another delimiter
				delimiter = '/';

				// first char not "/", e.g. "//node1" -> 2
				first_char = _ssp.find_first_not_of(delimiter);

				// only "/" ? thats bad!
				if (first_char == std::string::npos)
					throw ibrcommon::Exception("wrong eid format");
			}

			// start of application part
			size_t application_start = _ssp.find_first_of(delimiter, first_char);

			// no application part available
			if (application_start == std::string::npos)
				return "";

			if (_scheme == EID::CBHE_SCHEME)
			{
				// return the application part
				return _ssp.substr(application_start + 1, _ssp.length() - application_start - 1);
			}
			else
			{
				// return the application part
				return _ssp.substr(application_start, _ssp.length() - application_start);
			}
		}

		std::string EID::getNode() const throw (ibrcommon::Exception)
		{
			size_t first_char = 0;
			char delimiter = '.';

			if (_scheme != EID::CBHE_SCHEME)
			{
				// with a uncompressed bundle header we have another delimiter
				delimiter = '/';

				// first char not "/", e.g. "//node1" -> 2
				first_char = _ssp.find_first_not_of(delimiter);

				// only "/" ? thats bad!
				if (first_char == std::string::npos)
					throw ibrcommon::Exception("wrong eid format");
			}

			// start of application part
			size_t application_start = _ssp.find_first_of(delimiter, first_char);

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
			// with a compressed bundle header we have another delimiter
			if (_scheme == EID::CBHE_SCHEME)
			{
				return (_ssp.find_first_of(".") != std::string::npos);
			}

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

		bool EID::isCompressable() const
		{
			return ((_scheme == DEFAULT_SCHEME) && (_ssp == "none")) || (_scheme == EID::CBHE_SCHEME);
		}

		std::pair<size_t, size_t> EID::getCompressed() const
		{
			size_t node = 0;
			size_t app = 0;

			if (isCompressable())
			{
				std::stringstream ss_node(getNode());
				ss_node >> node;

				if (hasApplication())
				{
					std::stringstream ss_app(getApplication());
					ss_app >> app;
				}
			}

			return make_pair(node, app);
		}
	}
}
