/*
 * Registration.h
 *
 *  Created on: 15.06.2011
 *      Author: morgenro
 */

#ifndef REGISTRATION_H_
#define REGISTRATION_H_

#include "core/BundleStorage.h"
#include <ibrdtn/data/BundleID.h>
#include <ibrdtn/data/BundleList.h>
#include <ibrcommon/thread/Queue.h>
#include <string>
#include <set>

namespace dtn
{
	namespace api
	{
		class Registration
		{
		public:
			enum NOTIFY_CALL
			{
				NOTIFY_BUNDLE_AVAILABLE = 0,
				NOTIFY_NEIGHBOR_AVAILABLE = 1,
				NOTIFY_NEIGHBOR_UNAVAILABLE = 2,
				NOTIFY_SHUTDOWN = 3
			};

			/**
			 * constructor of the registration
			 */
			Registration();

			/**
			 * destructor of the registration
			 */
			virtual ~Registration();

			/**
			 * notify the corresponding client about something happen
			 */
			void notify(const NOTIFY_CALL);

			/**
			 * wait for the next notify event
			 * @return
			 */
			NOTIFY_CALL wait();

			/**
			 * wait for available bundle
			 */
			void wait_for_bundle(size_t timeout = 0);

			/**
			 * subscribe to a end-point
			 */
			void subscribe(const dtn::data::EID &endpoint);

			/**
			 * unsubscribe to a end-point
			 */
			void unsubscribe(const dtn::data::EID &endpoint);

			/**
			 * check if this registration has subscribed to a specific endpoint
			 * @param endpoint
			 * @return
			 */
			bool hasSubscribed(const dtn::data::EID &endpoint) const;

			/**
			 * @return A list of active subscriptions.
			 */
			const std::set<dtn::data::EID>& getSubscriptions() const;

			/**
			 * compares the local handle with the given one
			 */
			bool operator==(const std::string&) const;

			/**
			 * compares another registration with this one
			 */
			bool operator==(const Registration&) const;

			/**
			 * compares and order a registration (using the handle)
			 */
			bool operator<(const Registration&) const;

			/**
			 * Receive a bundle from the queue. If the queue is empty, it will
			 * query the storage for more bundles (up to 10). If no bundle is found
			 * and the queue is empty an exception is thrown.
			 * @return
			 */
			dtn::data::Bundle receive() throw (dtn::core::BundleStorage::NoBundleFoundException);

			dtn::data::MetaBundle receiveMetaBundle() throw (dtn::core::BundleStorage::NoBundleFoundException);

			/**
			 * notify a bundle as delivered (and delete it if singleton destination)
			 * @param id
			 */
			void delivered(const dtn::data::MetaBundle &m);

			/**
			 * returns the handle of this registration
			 * @return
			 */
			const std::string& getHandle() const;

			/**
			 * abort all blocking operations on this registration
			 */
			void abort();

		protected:
			void underflow();

		private:
			ibrcommon::Queue<dtn::data::MetaBundle> _queue;
			const std::string _handle;
			std::set<dtn::data::EID> _endpoints;
			dtn::data::BundleList _received_bundles;

			ibrcommon::Mutex _receive_lock;
			ibrcommon::Conditional _wait_for_cond;
			bool _no_more_bundles;

			ibrcommon::Queue<NOTIFY_CALL> _notify_queue;

			static const std::string alloc_handle();
			static void free_handle(const std::string &handle);

			static std::set<std::string> _handles;
		};
	}
}

#endif /* REGISTRATION_H_ */
