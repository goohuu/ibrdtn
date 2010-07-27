/*
 * BaseRouter.cpp
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#include "routing/BaseRouter.h"
#include "core/BundleCore.h"

#include "net/TransferAbortedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/BundleReceivedEvent.h"
#include "routing/QueueBundleEvent.h"
#include "routing/RequeueBundleEvent.h"
#include "core/BundleStorage.h"
#include "core/BundleGeneratedEvent.h"
#include "core/BundleExpiredEvent.h"
#include "core/BundleEvent.h"
#include "core/NodeEvent.h"
#include "core/TimeEvent.h"

#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>

namespace dtn
{
	namespace routing
	{
		BaseRouter *BaseRouter::Extension::_router = NULL;

		/**
		 * base implementation of the ThreadedExtension class
		 */
		BaseRouter::ThreadedExtension::ThreadedExtension()
		{ }

		BaseRouter::ThreadedExtension::~ThreadedExtension()
		{
			join();
		}

		/**
		 * base implementation of the Extension class
		 */
		BaseRouter::Extension::Extension()
		{ }

		BaseRouter::Extension::~Extension()
		{ }

		BaseRouter* BaseRouter::Extension::getRouter()
		{
			return _router;
		}

		/**
		 * base implementation of the Endpoint class
		 */
		BaseRouter::Endpoint::Endpoint()
		{ }

		BaseRouter::Endpoint::~Endpoint()
		{ }

		/**
		 * implementation of the VirtualEndpoint class
		 */
		BaseRouter::VirtualEndpoint::VirtualEndpoint(dtn::data::EID name)
		 : _client(NULL), _name(name)
		{ }

		BaseRouter::VirtualEndpoint::~VirtualEndpoint()
		{ }

		/**
		 * implementation of the BaseRouter class
		 */
		BaseRouter::BaseRouter(dtn::core::BundleStorage &storage)
		 : _storage(storage)
		{
			// register myself for all extensions
			Extension::_router = this;
		}

		BaseRouter::~BaseRouter()
		{
		}

		/**
		 * Add a routing extension to the routing core.
		 * @param extension
		 */
		void BaseRouter::addExtension(BaseRouter::Extension *extension)
		{
			_extensions.push_back(extension);
		}

		void BaseRouter::componentUp()
		{
			bindEvent(dtn::net::TransferAbortedEvent::className);
			bindEvent(dtn::net::TransferCompletedEvent::className);
			bindEvent(dtn::net::BundleReceivedEvent::className);
			bindEvent(dtn::routing::QueueBundleEvent::className);
			bindEvent(dtn::routing::RequeueBundleEvent::className);
			bindEvent(dtn::core::NodeEvent::className);
			bindEvent(dtn::core::BundleExpiredEvent::className);
			bindEvent(dtn::core::TimeEvent::className);
			bindEvent(dtn::core::BundleGeneratedEvent::className);

			for (std::list<BaseRouter::Extension*>::iterator iter = _extensions.begin(); iter != _extensions.end(); iter++)
			{
				ThreadedExtension *thread = dynamic_cast<ThreadedExtension*>(*iter);

				if (thread != NULL)
				{
					// run the thread
					thread->start();
				}
			}
		}

		void BaseRouter::componentDown()
		{
			unbindEvent(dtn::net::TransferAbortedEvent::className);
			unbindEvent(dtn::net::TransferCompletedEvent::className);
			unbindEvent(dtn::net::BundleReceivedEvent::className);
			unbindEvent(dtn::routing::QueueBundleEvent::className);
			unbindEvent(dtn::routing::RequeueBundleEvent::className);
			unbindEvent(dtn::core::NodeEvent::className);
			unbindEvent(dtn::core::BundleExpiredEvent::className);
			unbindEvent(dtn::core::TimeEvent::className);
			unbindEvent(dtn::core::BundleGeneratedEvent::className);

			// delete all extensions
			for (std::list<BaseRouter::Extension*>::iterator iter = _extensions.begin(); iter != _extensions.end(); iter++)
			{
				delete (*iter);
			}
		}

		/**
		 * Transfer one bundle to another node.
		 * @throw BundleNotFoundException if the bundle do not exist.
		 * @param destination The EID of the other node.
		 * @param id The ID of the bundle to transfer. This bundle must be stored in the storage.
		 */
		void BaseRouter::transferTo(const dtn::data::EID &destination, const dtn::data::BundleID &id)
		{
			// get the bundle out of the storage
			dtn::data::Bundle b = _storage.get(id);

			// send the bundle
			transferTo(destination, b);
		}

		void BaseRouter::transferTo(const dtn::data::EID &destination, dtn::data::Bundle &bundle)
		{
			// send the bundle
			dtn::core::BundleCore::getInstance().transferTo(destination, bundle);
		}

		/**
		 * method to receive new events from the EventSwitch
		 */
		void BaseRouter::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::core::TimeEvent &time = dynamic_cast<const dtn::core::TimeEvent&>(*evt);
				ibrcommon::MutexLock l(_known_bundles_lock);
				_known_bundles.expire(time.getTimestamp());

			} catch (std::bad_cast) {
			}

			try {
				const dtn::net::BundleReceivedEvent &received = dynamic_cast<const dtn::net::BundleReceivedEvent&>(*evt);

				// Store incoming bundles into the storage
				try {
					// store the bundle into a storage module
					dtn::core::BundleCore::getInstance().getStorage().store(received.bundle);

					// set the bundle as known
					if (!isKnown(received.bundle))
					{
						ibrcommon::MutexLock l(_known_bundles_lock);
						_known_bundles.add(received.bundle);

						// raise the queued event to notify all receivers about the new bundle
						QueueBundleEvent::raise(received.bundle);
					}

					// finally create a bundle received event
					dtn::core::BundleEvent::raise(received.bundle, dtn::core::BUNDLE_RECEIVED);

				} catch (ibrcommon::IOException ex) {
					IBRCOMMON_LOGGER(notice) << "Unable to store bundle " << received.bundle.toString() << IBRCOMMON_LOGGER_ENDL;
				} catch (dtn::core::BundleStorage::StorageSizeExeededException ex) {
					IBRCOMMON_LOGGER(notice) << "No space left for bundle " << received.bundle.toString() << IBRCOMMON_LOGGER_ENDL;
				}

				return;
			} catch (std::bad_cast) {
			}

			try {
				const dtn::core::BundleGeneratedEvent &generated = dynamic_cast<const dtn::core::BundleGeneratedEvent&>(*evt);

				// set the bundle as known
				{
					ibrcommon::MutexLock l(_known_bundles_lock);
					_known_bundles.add(generated.bundle);
				}

				// Store incoming bundles into the storage
				try {
					// store the bundle into a storage module
					dtn::core::BundleCore::getInstance().getStorage().store(generated.bundle);

					// raise the queued event to notify all receivers about the new bundle
					QueueBundleEvent::raise(generated.bundle);

				} catch (ibrcommon::IOException ex) {
					IBRCOMMON_LOGGER(notice) << "Unable to store bundle " << generated.bundle.toString() << IBRCOMMON_LOGGER_ENDL;
				} catch (dtn::core::BundleStorage::StorageSizeExeededException ex) {
					IBRCOMMON_LOGGER(notice) << "No space left for bundle " << generated.bundle.toString() << IBRCOMMON_LOGGER_ENDL;
				}

				return;
			} catch (std::bad_cast) {
			}

			try {
				const QueueBundleEvent &queued = dynamic_cast<const QueueBundleEvent&>(*evt);

				const dtn::data::EID &dest = queued.bundle.destination;

				if ( (dest.getNodeEID() == BundleCore::local.getNodeEID()) && dest.hasApplication() )
				{
					// if the bundle is for a local application, do not forward it to routing modules
					return;
				}
			} catch (std::bad_cast) {
			}

			// notify all underlying extensions
			for (std::list<BaseRouter::Extension*>::iterator iter = _extensions.begin(); iter != _extensions.end(); iter++)
			{
				(*iter)->notify(evt);
			}
		}

		/**
		 * Get a bundle out of the storage.
		 * @param id The ID of the bundle.
		 * @return The requested bundle.
		 */
		dtn::data::Bundle BaseRouter::getBundle(const dtn::data::BundleID &id)
		{
			return _storage.get(id);
		}

		dtn::core::BundleStorage& BaseRouter::getStorage()
		{
			return _storage;
		}

		// set the bundle as known
		bool BaseRouter::isKnown(const dtn::data::BundleID &id)
		{
			ibrcommon::MutexLock l(_known_bundles_lock);
			return _known_bundles.contains(id);
		}

		const SummaryVector BaseRouter::getSummaryVector()
		{
			ibrcommon::MutexLock l(_known_bundles_lock);
			return _known_bundles.getSummaryVector();
		}
	}
}
