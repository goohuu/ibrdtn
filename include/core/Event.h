/*
 * Event.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENT_H_
#define EVENT_H_

#include <string>

using namespace std;

namespace dtn
{
	namespace core
	{
		class Event
		{
		public:
			virtual const string getName() const = 0;

#ifdef DO_DEBUG_OUTPUT
			virtual string toString() = 0;
#endif
		};
	}
}


#endif /* EVENT_H_ */
