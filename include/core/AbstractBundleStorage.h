/*
 * AbstractBundleStorage.h
 *
 *  Created on: 24.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef ABSTRACTBUNDLESTORAGE_H_
#define ABSTRACTBUNDLESTORAGE_H_

#include "core/BundleCore.h"
#include "core/EventReceiver.h"
#include "core/BundleSchedule.h"
#include "data/Bundle.h"
#include "core/Node.h"

#include "utils/Conditional.h"

using namespace dtn::utils;

namespace dtn
{
	namespace core
	{
		class AbstractBundleStorage : public EventReceiver
		{
		public:
			/**
			 * store a schedule for later use
			 * @param schedule the schedule to store
			 */
			virtual void store(const BundleSchedule &schedule) = 0;

			/**
			 * Store a bundle fragment in the storage. If all fragments are available
			 * in the storage, they are merged and return by this method. If not, only
			 * NULL is returned.
			 */
			virtual Bundle* storeFragment(const Bundle &bundle) = 0;

			/**
			 * Clears all bundles, schedules and fragments in the storage.
			 */
			virtual void clear() {};

			/**
			 * @return True, if no bundles in the storage.
			 */
			virtual bool isEmpty() { return true; };

			/**
			 * @return the count of bundles in the storage
			 */
			virtual unsigned int getCount() { return 0; };

			/**
			 * receive event from the event switch
			 */
			void raiseEvent(const Event *evt);

		protected:
			/**
			 * constructor
			 */
			AbstractBundleStorage();

			/**
			 * destructor
			 */
			~AbstractBundleStorage();

			/**
			 * wait till time has changed
			 */
			void wait();

			/**
			 * break the wait
			 */
			void stopWait();

			virtual void eventNodeAvailable(const Node &node) {};
			virtual void eventNodeUnavailable(const Node &node) {};

		private:
			Conditional m_breakwait;
		};
	}
}

#endif /* ABSTRACTBUNDLESTORAGE_H_ */
