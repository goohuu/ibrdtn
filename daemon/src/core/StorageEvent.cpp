/*
 * StorageEvent.cpp
 *
 *  Created on: 04.02.2010
 *      Author: Myrtus
 */

#include "core/StorageEvent.h"

using namespace dtn::core;

namespace dtn
{
	namespace core
	{
		StorageEvent::StorageEvent(const string id, int errcode, EventStorageAction action): bundlid(id), errorcode(errcode), event_action(action)
		{
		}

		StorageEvent::~StorageEvent(){}

		const string StorageEvent::getName() const{
			return StorageEvent::className;
		}

		EventStorageAction StorageEvent::getAction() const{
			return event_action;
		}

		const string StorageEvent::getBundleID() const{
			return bundlid;
		}

		const int StorageEvent::getError() const{
			return errorcode;
		}

		void StorageEvent::raise(const string id, const int errcode, const EventStorageAction action)
		{
			// raise the new event
			raiseEvent( new StorageEvent(id,errcode,action) );
		}

		const string StorageEvent::className = "StorageEvent";

		#ifdef DO_DEBUG_OUTPUT
			string StorageEvent::toString() const
			{
				return className;
			}
		#endif
	}
}
