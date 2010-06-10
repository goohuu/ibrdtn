/*
 * EID.h
 *
 *  Created on: 09.03.2009
 *      Author: morgenro
 */

#ifndef EID_H_
#define EID_H_

#include <string>
#include "ibrcommon/Exceptions.h"

using namespace std;

namespace dtn {
namespace data
{
	class EID
	{
	public:
		EID();
		EID(std::string scheme, std::string ssp);
		EID(std::string value);
		virtual ~EID();

		EID(const EID &other);

		EID& operator=(const EID &other);

		bool operator==(EID const& other) const;

		bool operator==(string const& other) const;

		bool operator!=(EID const& other) const;

		EID operator+(string suffix);

		bool sameHost(string const& other) const;
		bool sameHost(EID const& other) const;

		bool operator<(EID const& other) const;
		bool operator>(const EID& other) const;

		string getString() const;
		string getApplication() const throw (ibrcommon::Exception);
		string getNode() const throw (ibrcommon::Exception);
		string getScheme() const;

		string getNodeEID() const;

		bool hasApplication() const;

	private:
		std::string _scheme;
		std::string _ssp;
	};
}
}

#endif /* EID_H_ */
