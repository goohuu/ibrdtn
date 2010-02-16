/*
 * BundleExpiredEvent.h
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#ifndef BUNDLEEXPIREDEVENT_H_
#define BUNDLEEXPIREDEVENT_H_

#include "core/Event.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BundleID.h"
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace core
	{
		class BundleExpiredEvent : public dtn::core::Event
		{
		public:
			virtual ~BundleExpiredEvent();

			const string getName() const;

#ifdef DO_DEBUG_OUTPUT
			string toString() const;
#endif

			static const string className;

			static void raise(const dtn::data::Bundle &bundle);
			dtn::data::BundleID _bundle;

		private:
			BundleExpiredEvent(const dtn::data::Bundle &bundle);
		};
	}
}

#endif /* BUNDLEEXPIREDEVENT_H_ */
