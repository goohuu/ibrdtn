/*
 * BaseRouter.h
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#ifndef BASEROUTER_H_
#define BASEROUTER_H_

#include "ibrdtn/data/DTNTime.h"
#include "ibrdtn/data/BundleID.h"
#include "core/EventReceiver.h"
#include "core/BundleStorage.h"
#include "ibrcommon/thread/Thread.h"
#include "ibrcommon/thread/Conditional.h"
#include "routing/MetaBundle.h"
#include <list>
#include <queue>

namespace dtn
{
	namespace routing
	{
		class BaseRouter : public dtn::core::EventReceiver
		{
		public:
			class Extension
			{
			public:
				Extension();
				virtual ~Extension() = 0;

				virtual void notify(const dtn::core::Event *evt) = 0;

			protected:
				BaseRouter *getRouter();

			private:
				friend class BaseRouter;
				static BaseRouter *_router;
			};

			class ThreadedExtension : public Extension, public ibrcommon::JoinableThread
			{
			public:
				ThreadedExtension();
				virtual ~ThreadedExtension() = 0;

				virtual void notify(const dtn::core::Event *evt) = 0;

			protected:
				void stopExtension();
				bool _running;
				ibrcommon::Conditional _wait;
			};

			class Endpoint
			{
			public:
				Endpoint();
				virtual ~Endpoint() = 0;
			};

		private:
//			class MetaBundle : public dtn::data::BundleID
//			{
//			public:
//				MetaBundle(const dtn::data::Bundle &b);
//				~MetaBundle();
//
//				dtn::data::DTNTime _received;
//				dtn::data::DTNTime _schedule;
//				size_t _lifetime;
//
//				enum State {
//					TRANSIT = 0,
//					WAIT_ENDPOINT = 1,
//					DELAYED = 2
//				};
//
//				State _state;
//			};

			class SeenBundle : public dtn::data::BundleID
			{
			public:
				SeenBundle(const dtn::data::Bundle &b);
				virtual ~SeenBundle();

				dtn::data::DTNTime _lastseen;
				dtn::data::DTNTime _expire;
			};

			class VirtualEndpoint
			{
			public:
				VirtualEndpoint(dtn::data::EID name);
				virtual ~VirtualEndpoint();

				ibrcommon::Mutex _clientlock;
				Endpoint *_client;

				dtn::data::EID _name;
				dtn::data::DTNTime _lastseen;
				dtn::data::DTNTime _expire;
			};

		public:
			BaseRouter(dtn::core::BundleStorage &storage);
			~BaseRouter();

			/**
			 * Add a routing extension to the routing core.
			 * @param extension
			 */
			void addExtension(BaseRouter::Extension *extension);

			/**
			 * Transfer one bundle to another node.
			 * @throw BundleNotFoundException if the bundle do not exist.
			 * @param destination The EID of the other node.
			 * @param id The ID of the bundle to transfer. This bundle must be stored in the storage.
			 */
			void transferTo(const dtn::data::EID &destination, const dtn::data::BundleID &id);
			void transferTo(const dtn::data::EID &destination, dtn::data::Bundle &bundle);

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const dtn::core::Event *evt);

			/**
			 * Check if one bundle was seen before.
			 * @param id The ID of the Bundle.
			 * @return True, if the bundle was seen before. False if not.
			 */
			bool wasSeenBefore(const dtn::data::BundleID &id) const;

			/**
			 * Get a bundle out of the storage.
			 * @param id The ID of the bundle.
			 * @return The requested bundle.
			 */
			dtn::data::Bundle getBundle(const dtn::data::BundleID &id);

			dtn::core::BundleStorage &getStorage();

			bool isLocal(const dtn::data::Bundle &b) const;

			bool isLocal(const dtn::routing::MetaBundle &b) const;

		private:
			dtn::core::BundleStorage &_storage;
			std::list<BaseRouter::Extension*> _extensions;

			std::list<SeenBundle> _seenlist;
			//std::queue<MetaBundle> _delayed;
		};
	}
}


#endif /* BASEROUTER_H_ */
