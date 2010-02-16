/*
 * TransferCompletedEvent.h
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef TRANSFERCOMPLETEDEVENT_H_
#define TRANSFERCOMPLETEDEVENT_H_

#include "core/Event.h"
#include "ibrdtn/data/BundleID.h"
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace net
	{
		class TransferCompletedEvent : public dtn::core::Event
		{
		public:
			virtual ~TransferCompletedEvent();

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
			TransferCompletedEvent(const dtn::data::EID peer, const dtn::data::Bundle &bundle);
		};
	}
}

#endif /* TRANSFERCOMPLETEDEVENT_H_ */
