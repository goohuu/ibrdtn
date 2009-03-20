/*
 * EID.cpp
 *
 *  Created on: 09.03.2009
 *      Author: morgenro
 */

#include "data/EID.h"

namespace dtn
{
	namespace data
	{
		EID::EID()
		: m_value("dtn:none"), m_application(""), m_node("none"), m_scheme("dtn")
		{
		}

		EID::EID(string value)
		: m_value(value)
		{
			m_scheme = m_value.substr(0, m_value.find_first_of(":"));

			string ssp = m_value.substr(m_scheme.length() + 1, m_value.length() - m_scheme.length() - 1);

			// first char not "/", e.g. "//node1" -> 2
			size_t first_char = ssp.find_first_not_of("/");

			// substring has no trailing "/", e.g. "//node1/test" -> "node1/test"
			string tmp = ssp.substr(first_char, ssp.length() - first_char);

			// start of application part
			size_t application_start = ssp.find_first_of("/", first_char);

			// extract application
			m_application = ssp.substr(application_start, ssp.length() - application_start);

			// extract node
			m_node = ssp.substr(0, application_start);
		}

		EID::~EID()
		{
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
