/*
 * TransferCompletedEvent.h
 *
 *  Created on: 16.02.2010
 *      Author: morgenro
 */

#ifndef TRANSFERCOMPLETEDEVENT_H_
#define TRANSFERCOMPLETEDEVENT_H_

#include "core/Event.h"
#include "routing/MetaBundle.h"
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

			string toString() const;

			static const string className;

			static void raise(const dtn::data::EID peer, const dtn::data::Bundle &bundle);

			dtn::data::EID getPeer() const;
			dtn::routing::MetaBundle getBundle() const;

		private:
			dtn::data::EID _peer;
			dtn::routing::MetaBundle _bundle;
			TransferCompletedEvent(const dtn::data::EID peer, const dtn::data::Bundle &bundle);
		};
	}
}

#endif /* TRANSFERCOMPLETEDEVENT_H_ */
