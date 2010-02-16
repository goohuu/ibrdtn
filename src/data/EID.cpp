/*
 * EID.cpp
 *
 *  Created on: 09.03.2009
 *      Author: morgenro
 */

#include "ibrdtn/data/EID.h"

namespace dtn {
namespace data
{
	EID::EID()
	: m_value("dtn:none"), m_scheme("dtn"), m_node("none"), m_application("")
	{
	}

	EID::EID(const EID &other)
	 : m_value(other.m_value), m_scheme(other.m_scheme), m_node(other.m_node), m_application(other.m_application)
	{
	}

	EID::EID(string scheme, string ssp)
	 : m_value(scheme + ":" + ssp), m_scheme(scheme), m_node(""), m_application("")
	{
		// first char not "/", e.g. "//node1" -> 2
		size_t first_char = ssp.find_first_not_of("/");

		// start of application part
		size_t application_start = ssp.find_first_of("/", first_char);

		if (application_start == string::npos)
		{
			m_application = "";
			m_node = ssp;
		}
		else
		{
			// extract application
			m_application = ssp.substr(application_start, ssp.length() - application_start);

			// extract node
			m_node = ssp.substr(0, application_start);
		}
	}

	EID::EID(string value)
	: m_value(value)
	{
		m_scheme = m_value.substr(0, m_value.find_first_of(":"));

		string ssp = m_value.substr(m_scheme.length() + 1, m_value.length() - m_scheme.length() - 1);

		// first char not "/", e.g. "//node1" -> 2
		size_t first_char = ssp.find_first_not_of("/");

		// start of application part
		size_t application_start = ssp.find_first_of("/", first_char);

		if (application_start == string::npos)
		{
			m_application = "";
			m_node = ssp;
		}
		else
		{
			// extract application
			m_application = ssp.substr(application_start, ssp.length() - application_start);

			// extract node
			m_node = ssp.substr(0, application_start);
		}
	}

	EID::~EID()
	{
	}

	EID& EID::operator=(const EID &other)
	{
		m_value = other.m_value;
		m_scheme = other.m_scheme;
		m_node = other.m_node;
		m_application = other.m_application;

		return *this;
	}

	bool EID::operator==(EID const& other) const
	{
		return equal(other);
	}

	bool EID::operator==(string const& other) const
	{
		return equal(other);
	}

	bool EID::operator!=(EID const& other) const
	{
		return !equal(other);
	}

	EID EID::operator+(string suffix)
	{
		return EID(m_value + suffix);
	}

	bool EID::equal(string const& other) const
	{
		if (m_value == other)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool EID::equal(EID const& other) const
	{
		if (m_value == other.m_value)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool EID::sameHost(string const& other) const
	{
		if ( other == getNodeEID() ) return true;
		return false;
	}

	bool EID::sameHost(EID const& other) const
	{
		if ( other.getNodeEID() == getNodeEID() ) return true;
		return false;
	}

	bool EID::operator<(EID const& other) const
	{
		return m_value < other.m_value;
	}

	bool EID::operator>(EID const& other) const
	{
		return m_value > other.m_value;
	}

	string EID::getString() const
	{
		return m_value;
	}

	string EID::getApplication() const
	{
		return m_application;
	}

	string EID::getNode() const
	{
		return m_node;
	}

	string EID::getScheme() const
	{
		return m_scheme;
	}

	string EID::getNodeEID() const
	{
		return m_scheme + ":" + m_node;
	}

	bool EID::hasApplication() const
	{
		return (m_application != "");
	}
}
}
