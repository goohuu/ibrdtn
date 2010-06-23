/*
 * QueueBundle.cpp
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#include "routing/QueueBundleEvent.h"
#include "core/BundleCore.h"

namespace dtn
{
	namespace routing
	{
		QueueBundleEvent::QueueBundleEvent(const dtn::data::Bundle &b)
		 : bundle(b)
		{

		}

		QueueBundleEvent::~QueueBundleEvent()
		{

		}

		void QueueBundleEvent::raise(const dtn::data::Bundle &bundle)
		{
			// raise the new event
			raiseEvent( new QueueBundleEvent(bundle) );
		}

		const string QueueBundleEvent::getName() const
		{
			return QueueBundleEvent::className;
		}

		string QueueBundleEvent::toString() const
		{
			return className + ": New bundle queued " + bundle.toString();
		}

		const string QueueBundleEvent::className = "QueueBundleEvent";
	}
}
