/*
 * Registration.h
 *
 *  Created on: 15.06.2011
 *      Author: morgenro
 */

#ifndef REGISTRATION_H_
#define REGISTRATION_H_

#include <ibrdtn/data/BundleID.h>
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
				NOTIFY_BUNDLE_QUEUED = 0,
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
			 * enqueue a bundle to the queue of this registration
			 * @param id
			 */
			void queue(const dtn::data::BundleID &id);

			/**
			 * notify the corresponding client about something happen
			 */
			void notify(const NOTIFY_CALL) { };

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
			 * @return The queue of all waiting bundles
			 */
			ibrcommon::Queue<dtn::data::BundleID>& getQueue();

			const std::string& getHandle() const;

		protected:

		private:
			ibrcommon::Queue<dtn::data::BundleID> _queue;
			const std::string _handle;
			std::set<dtn::data::EID> _endpoints;

			static const std::string alloc_handle();
			static void free_handle(const std::string &handle);

			static std::set<std::string> _handles;
		};
	}
}

#endif /* REGISTRATION_H_ */
