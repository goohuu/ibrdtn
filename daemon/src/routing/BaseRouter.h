/*
 * BaseRouter.h
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#ifndef BASEROUTER_H_
#define BASEROUTER_H_

#include "Component.h"
#include "ibrdtn/data/DTNTime.h"
#include "ibrdtn/data/BundleID.h"
#include "core/EventReceiver.h"
#include "core/BundleStorage.h"
#include "ibrcommon/thread/Thread.h"
#include "ibrcommon/thread/Conditional.h"
#include <ibrdtn/data/MetaBundle.h>
#include "routing/BundleSummary.h"

namespace dtn
{
	namespace routing
	{
		class BaseRouter : public dtn::core::EventReceiver, public dtn::daemon::IntegratedComponent
		{
		public:
			class RoutingException : public ibrcommon::Exception
			{
				public:
					RoutingException(string what = "An error occured during routing.") throw() : Exception(what)
					{
					};
			};

			/**
			 * If no neighbour was found, this exception is thrown.
			 */
			class NoNeighbourFoundException : public RoutingException
			{
				public:
					NoNeighbourFoundException(string what = "No neighbour was found.") throw() : RoutingException(what)
					{
					};
			};

			/**
			 * If no route can be found, this exception is thrown.
			 */
			class NoRouteFoundException : public RoutingException
			{
				public:
					NoRouteFoundException(string what = "No route found.") throw() : RoutingException(what)
					{
					};
			};

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
				virtual void stopExtension() = 0;
			};

			class Endpoint
			{
			public:
				Endpoint();
				virtual ~Endpoint() = 0;
			};

		private:
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

			/**
			 * method to receive new events from the EventSwitch
			 */
			void raiseEvent(const dtn::core::Event *evt);

			/**
			 * Get a bundle out of the storage.
			 * @param id The ID of the bundle.
			 * @return The requested bundle.
			 */
			dtn::data::Bundle getBundle(const dtn::data::BundleID &id);

			dtn::core::BundleStorage &getStorage();

			bool isKnown(const dtn::data::BundleID &id);

			const SummaryVector getSummaryVector();

		protected:
			virtual void componentUp();
			virtual void componentDown();

		private:
			ibrcommon::Mutex _known_bundles_lock;
			BundleSummary _known_bundles;

			dtn::core::BundleStorage &_storage;
			std::list<BaseRouter::Extension*> _extensions;
		};
	}
}


#endif /* BASEROUTER_H_ */
