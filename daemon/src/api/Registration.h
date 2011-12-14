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
#include <ibrcommon/thread/Mutex.h>
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

			class RegistrationException : public ibrcommon::Exception
			{
			public:
				RegistrationException(string what = "") throw() : Exception(what)
				{
				}
			};

			class AlreadyAttachedException : public RegistrationException
			{
			public:
				AlreadyAttachedException(string what = "") throw() : RegistrationException(what)
				{
				}
			};

			class NotFoundException : public RegistrationException
			{
			public:
				NotFoundException(string what = "") throw() : RegistrationException(what)
				{
				}
			};

			class NotPersistentException : public RegistrationException
			{
			public:
				NotPersistentException(string what = "") throw() : RegistrationException(what)
				{
				}
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
			bool hasSubscribed(const dtn::data::EID &endpoint);

			/**
			 * @return A list of active subscriptions.
			 */
			const std::set<dtn::data::EID> getSubscriptions();

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

			/**
			 * resets the underlying queues (if you want to continue
			 * using the registration after an abort call
			 */
			void reset();

			/**
			 * make this registration persistent
			 * @param lifetime the lifetime of the registration in seconds
			 */
			void setPersistent(size_t lifetime);

			/**
			 * remove the persistent flag of this registration
			 */
			void unsetPersistent();

			/**
			 * returns the persistent state of this registration
			 * and also sets the persistent state to false, if the timer is expired
			 * @return the persistent state
			 */
			bool isPersistent();

			/**
			 * returns the persistent state of this registration
			 * @return the persistent state
			 */
			bool isPersistent() const;

			/**
			 * gets the expire time of this registration if it is persistent
			 * @see ibrcommon::Timer::get_current_time()
			 * @exception NotPersistentException the registration is not persistent
			 * @return the expire time according to ibrcommon::Timer::get_current_time()
			 */
			size_t getExpireTime() const;

			/**
			 * attaches a client to this registration
			 * @exception AlreadyAttachedException a client is already attached
			 */
			void attach();

			/**
			 * detaches a client from this registration
			 */
			void detach();

		protected:
			void underflow();

		private:
			ibrcommon::Queue<dtn::data::MetaBundle> _queue;
			const std::string _handle;
			ibrcommon::Mutex _endpoints_lock;
			std::set<dtn::data::EID> _endpoints;
			dtn::data::BundleList _received_bundles;

			ibrcommon::Mutex _receive_lock;
			ibrcommon::Conditional _wait_for_cond;
			bool _no_more_bundles;

			ibrcommon::Queue<NOTIFY_CALL> _notify_queue;

			static const std::string alloc_handle();
			static void free_handle(const std::string &handle);

			static std::set<std::string> _handles;

			bool _persistent;
			bool _detached;
			ibrcommon::Mutex _attach_lock;
			size_t _expiry;
		};
	}
}

#endif /* REGISTRATION_H_ */
