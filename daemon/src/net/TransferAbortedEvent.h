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
#include <string>

namespace dtn
{
	namespace net
	{
		class TransferAbortedEvent : public dtn::core::Event
		{
		public:
			enum AbortReason
			{
				REASON_UNDEFINED = 0,
				REASON_CONNECTION_DOWN = 1,
				REASON_REFUSED = 2,
				REASON_RETRY_LIMIT_REACHED = 3,
				REASON_BUNDLE_DELETED = 4
			};

			virtual ~TransferAbortedEvent();

			const std::string getName() const;

			string toString() const;

			static const std::string className;

			static void raise(const dtn::data::EID &peer, const dtn::data::Bundle &bundle, const AbortReason reason = REASON_UNDEFINED);
			static void raise(const dtn::data::EID &peer, const dtn::data::BundleID &id, const AbortReason reason = REASON_UNDEFINED);

			const dtn::data::EID& getPeer() const;
			const dtn::data::BundleID& getBundleID() const;

			const AbortReason reason;

		private:
			static const std::string getReason(const AbortReason reason);

			const dtn::data::EID _peer;
			const dtn::data::BundleID _bundle;
			TransferAbortedEvent(const dtn::data::EID &peer, const dtn::data::Bundle &bundle, const AbortReason reason);
			TransferAbortedEvent(const dtn::data::EID &peer, const dtn::data::BundleID &id, const AbortReason reason);
		};
	}
}

#endif /* TRANSFERABORTEDEVENT_H_ */
