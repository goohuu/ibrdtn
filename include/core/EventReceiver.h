/*
 * EventReceiver.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#ifndef EVENTRECEIVER_H_
#define EVENTRECEIVER_H_

namespace dtn
{
	namespace core
	{
		class Event;

		class EventReceiver
		{
		public:
			virtual void raiseEvent(const Event *evt) = 0;
		};
	}
}

#endif /* EVENTRECEIVER_H_ */
