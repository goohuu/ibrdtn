/*
 * StorageEvent.h
 *
 *  Created on: 04.02.2010
 *      Author: Myrtus
 */

#ifndef STORAGEEVENT_H_
#define STORAGEEVENT_H_

#include "core/Event.h"
#include "ibrdtn/data/BundleID.h"

namespace dtn
{
	namespace core
	{
		enum EventStorageAction
		{
			BUNDLE_SUCCESSFUL_DELIVERED = 0,
			BUNDLE_DELETED = 1,
		};

		class StorageEvent : public Event{
		public:

			StorageEvent(const string id, int errcode, EventStorageAction action);
			virtual ~StorageEvent();

			const std::string getName() const;

			EventStorageAction getAction() const;

			const std::string getBundleID() const;

			const int getError() const;

			static void raise(const string id, int errcode, EventStorageAction action);

			static const string className;

			std::string toString() const;

		private:
				const string bundlid;
				const int errorcode;
				const EventStorageAction event_action;
		};
	}
}

#endif /* STORAGEEVENT_H_ */
