/*
 * EID.h
 *
 *  Created on: 09.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef EID_H_
#define EID_H_

#include <string>

using namespace std;

namespace dtn
{
	namespace data
	{
		class EID
		{
		public:
			EID();
			EID(string value);
			~EID();

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
