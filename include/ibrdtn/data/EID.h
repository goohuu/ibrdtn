/*
 * EID.h
 *
 *  Created on: 09.03.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef EID_H_
#define EID_H_

#include <string>

using namespace std;

namespace dtn {
namespace data
{
	class EID
	{
	public:
		EID();
		EID(string scheme, string ssp);
		EID(string value);
		virtual ~EID();

		bool operator==(EID const& other) const
		{
			return equal(other);
		}

		bool operator==(string const& other) const
		{
			return equal(other);
		}

		bool operator!=(EID const& other) const
		{
			return !equal(other);
		}

		EID operator+(string suffix);

		bool equal(string const& other) const;
		bool equal(EID const& other) const;

		bool sameHost(string const& other) const;
		bool sameHost(EID const& other) const;

		bool operator<(EID const& other) const;
		bool operator>(const EID& other) const;

		string getString() const;
		string getApplication() const;
		string getNode() const;
		string getScheme() const;

		string getNodeEID() const;

		bool hasApplication() const;

	private:
		string m_value;
		string m_scheme;
		string m_node;
		string m_application;
	};
}
}

#endif /* EID_H_ */
