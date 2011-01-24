/*
 * EpidemicControlMessage.h
 *
 *  Created on: 24.01.2011
 *      Author: morgenro
 */

#ifndef EPIDEMICCONTROLMESSAGE_H_
#define EPIDEMICCONTROLMESSAGE_H_

#include <iostream>

namespace dtn
{
	namespace routing
	{
		class EpidemicControlMessage
		{
		public:
			EpidemicControlMessage();
			virtual ~EpidemicControlMessage();

			friend std::ostream& operator<<(std::ostream&, const EpidemicControlMessage&);
			friend std::istream& operator>>(std::istream&, EpidemicControlMessage&);
		};
	}
}

#endif /* EPIDEMICCONTROLMESSAGE_H_ */
