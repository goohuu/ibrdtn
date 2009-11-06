/*
 * BundleStorage.h
 *
 *  Created on: 24.03.2009
 *      Author: morgenro
 */

#ifndef BUNDLESTORAGE_H_
#define BUNDLESTORAGE_H_

#include "ibrdtn/default.h"
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
		protected:
			class Item
			{
			public:
				virtual ~Item() {};
				virtual dtn::data::Bundle &getBundle() = 0;
			};

		public:
			class Iterator
			{
			public:
				Iterator(BundleStorage::Item *i) : _item(i) {}
				virtual ~Iterator() {}

				// The assignment and relational operators are straightforward
				Iterator& operator=(const Iterator& other)
				{
					_item = other._item;
					return(*this);
				}

				bool operator==(const Iterator& other)
				{
					return(_item == other._item);
				}

				bool operator!=(const Iterator& other)
				{
					return(_item != other._item);
				}

				// Get the next element.
				virtual Iterator& operator++() = 0;

				virtual Iterator& operator++(int) = 0;
//					Iterator tmp(*this);
//					++(*this);
//					return(tmp);
//				}

				// Return a reference to the value in the node.  I do this instead
				// of returning by value so a caller can update the value in the
				// node directly.
				dtn::data::Bundle& operator*()
				{
					return(_item->getBundle());
				}

				// Return the address of the value referred to.
				virtual dtn::data::Bundle* operator->() = 0;
//				{
//					return(&*(BundleStorage::Iterator)*this);
//				}

			private:
				BundleStorage::Item *_item;

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
			 * If there is no bundle available the method will block until
			 * a global shutdown or a bundle is available.
			 * @param eid The receiver for the bundle.
			 * @return A bundle object.
			 */
			virtual dtn::data::Bundle get(const dtn::data::EID &eid) = 0;

			/**
			 * Unblock the get()-call for a specific EID.
			 * @param eid
			 */
			virtual void unblock(const dtn::data::EID &eid) = 0;

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

		protected:
			/**
			 * constructor
			 */
			BundleStorage();
		};
	}
}

#endif /* BUNDLESTORAGE_H_ */
