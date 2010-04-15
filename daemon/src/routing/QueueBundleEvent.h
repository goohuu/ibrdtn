/*
 * QueueBundle.h
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#ifndef QUEUEBUNDLEEVENT_H_
#define QUEUEBUNDLEEVENT_H_

#include "core/Event.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BundleID.h"
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace routing
	{
		class QueueBundleEvent : public dtn::core::Event
		{
		public:
			virtual ~QueueBundleEvent();

			const string getName() const;

#ifdef DO_DEBUG_OUTPUT
			string toString() const;
#endif

			static const string className;

			static void raise(const dtn::data::Bundle &bundle);
			dtn::data::BundleID _bundle;

		private:
			QueueBundleEvent(const dtn::data::Bundle &bundle);
		};
	}
}

#endif /* QUEUEBUNDLEEVENT_H_ */
