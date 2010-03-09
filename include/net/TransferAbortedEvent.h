/*
 * TransferAbortedEvent.h
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef TRANSFERABORTEDEVENT_H_
#define TRANSFERABORTEDEVENT_H_

#include "core/Event.h"
#include "ibrdtn/data/BundleID.h"
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace net
	{
		class TransferAbortedEvent : public dtn::core::Event
		{
		public:
			virtual ~TransferAbortedEvent();

			const string getName() const;

#ifdef DO_DEBUG_OUTPUT
			string toString() const;
#endif

			static const string className;

			static void raise(const dtn::data::EID &peer, const dtn::data::Bundle &bundle);
			static void raise(const dtn::data::EID &peer, const dtn::data::BundleID &id);

			dtn::data::EID getPeer() const;
			dtn::data::BundleID getBundleID() const;

		private:
			const dtn::data::EID _peer;
			const dtn::data::BundleID _bundle;
			TransferAbortedEvent(const dtn::data::EID &peer, const dtn::data::Bundle &bundle);
			TransferAbortedEvent(const dtn::data::EID &peer, const dtn::data::BundleID &id);
		};
	}
}

#endif /* TRANSFERABORTEDEVENT_H_ */
