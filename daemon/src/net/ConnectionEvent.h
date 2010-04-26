/*
 * ConnectionEvent.h
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef CONNECTIONEVENT_H_
#define CONNECTIONEVENT_H_

#include "core/Event.h"
#include "ibrdtn/data/EID.h"
#include "net/BundleConnection.h"

namespace dtn
{
	namespace net
	{
		class ConnectionEvent : public dtn::core::Event
		{
		public:
			enum State
			{
				CONNECTION_SETUP = 0,
				CONNECTION_UP = 1,
				CONNECTION_DOWN = 2,
				CONNECTION_TIMEOUT = 3
			};

			~ConnectionEvent();

			const string getName() const;

#ifdef DO_DEBUG_OUTPUT
			string toString() const;
#endif

			static const string className;

			static void raise(State state, const dtn::data::EID &peer);

			const State state;
			const dtn::data::EID peer;

		private:
			ConnectionEvent(State state, const dtn::data::EID &peer);
		};
	}
}


#endif /* CONNECTIONEVENT_H_ */
