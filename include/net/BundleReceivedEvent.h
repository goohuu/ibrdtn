/*
 * BundleReceivedEvent.h
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#ifndef BUNDLERECEIVEDEVENT_H_
#define BUNDLERECEIVEDEVENT_H_

#include "core/Event.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BundleID.h"
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace net
	{
		class BundleReceivedEvent : public dtn::core::Event
		{
		public:
			virtual ~BundleReceivedEvent();

			const string getName() const;

#ifdef DO_DEBUG_OUTPUT
			string toString() const;
#endif

			static const string className;

			static void raise(const dtn::data::EID peer, const dtn::data::Bundle &bundle);

			dtn::data::EID getPeer() const;
			dtn::data::BundleID getBundleID() const;

		private:
			dtn::data::EID _peer;
			dtn::data::BundleID _bundle;
			BundleReceivedEvent(const dtn::data::EID peer, const dtn::data::Bundle &bundle);
		};
	}
}


#endif /* BUNDLERECEIVEDEVENT_H_ */
