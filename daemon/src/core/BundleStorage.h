/*
 * BundleStorage.h
 *
 *  Created on: 24.03.2009
 *      Author: morgenro
 */

#ifndef BUNDLESTORAGE_H_
#define BUNDLESTORAGE_H_

#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/BundleID.h"

#include <stdexcept>
#include <iterator>

namespace dtn
{
	namespace core
	{
		class BundleStorage
		{
		public:
			class NoBundleFoundException : public dtn::MissingObjectException
			{
			public:
				NoBundleFoundException(string what = "No bundle match the specified criteria.") throw() : dtn::MissingObjectException(what)
				{
				};
			};

			class StorageSizeExeededException : public ibrcommon::Exception
			{
			public:
				StorageSizeExeededException(string what = "No space left in the storage.") throw() : ibrcommon::Exception(what)
				{
				};
			};

			/**
			 * destructor
			 */
			virtual ~BundleStorage() = 0;

			/**
			 * Stores a bundle in the storage.
			 * @param bundle The bundle to store.
			 */
			virtual void store(const dtn::data::Bundle &bundle) = 0;

			/**
			 * This method returns a specific bundle which is identified by
			 * its id.
			 * @param id The ID of the bundle to return.
			 * @return A bundle object.
			 */
			virtual dtn::data::Bundle get(const dtn::data::BundleID &id) = 0;

			/**
			 * This method returns a bundle which is addressed to a EID.
			 * @param eid The receiver for the bundle.
			 * @return A bundle object.
			 */
			virtual dtn::data::Bundle get(const dtn::data::EID &eid) = 0;

			/**
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param id The ID of the bundle to remove.
			 */
			virtual void remove(const dtn::data::BundleID &id) = 0;

			/**
			 * This method deletes a specific bundle in the storage.
			 * No reports will be generated here.
			 * @param b The bundle to remove.
			 */
			void remove(const dtn::data::Bundle &b);

			/**
			 * Clears all bundles and fragments in the storage.
			 */
			virtual void clear() {};

			/**
			 * @return True, if no bundles in the storage.
			 */
			virtual bool empty() { return true; };

			/**
			 * @return the count of bundles in the storage
			 */
			virtual unsigned int count() { return 0; };

			/**
			 * This method is called if another node accepts custody for a
			 * bundle of us.
			 * @param bundle
			 */
			virtual void releaseCustody(dtn::data::BundleID &bundle) = 0;

			/**
			 * Accept custody for a given bundle. The bundle is modified by this method
			 * and contains us as future custodian. The previous custodian gets notified
			 * with a custody accept message.
			 * @param bundle
			 */
			void acceptCustody(dtn::data::Bundle &bundle);

			/**
			 * Reject custody for a given bundle. The custodian of this bundle gets notified
			 * with a custody reject message.
			 * @param bundle
			 */
			void rejectCustody(const dtn::data::Bundle &bundle);

		protected:
			/**
			 * constructor
			 */
			BundleStorage();
		};
	}
}

#endif /* BUNDLESTORAGE_H_ */
