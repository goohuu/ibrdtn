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
#include "ibrdtn/data/MetaBundle.h"

namespace dtn
{
	namespace net
	{
		class BundleReceivedEvent : public dtn::core::Event
		{
		public:
			virtual ~BundleReceivedEvent();

			const string getName() const;

			string toString() const;

			static const string className;

			static void raise(const dtn::data::EID peer, const dtn::data::Bundle &bundle);

			dtn::data::EID getPeer() const;
			dtn::data::MetaBundle getBundle() const;

		private:
			dtn::data::EID _peer;
			dtn::data::MetaBundle _bundle;
			BundleReceivedEvent(const dtn::data::EID peer, const dtn::data::Bundle &bundle);
		};
	}
}


#endif /* BUNDLERECEIVEDEVENT_H_ */
