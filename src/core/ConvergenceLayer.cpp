#include "core/ConvergenceLayer.h"
#include "core/BundleReceiver.h"
#include "core/EventSwitch.h"
#include "core/RouteEvent.h"
#include "core/BundleEvent.h"

namespace dtn
{
	namespace core
	{
		void ConvergenceLayer::setBundleReceiver(BundleReceiver *receiver)
		{
			m_receiver = receiver;
		}

		void ConvergenceLayer::eventBundleReceived(const Bundle &bundle)
		{
			if (m_receiver != NULL)
			{
				m_receiver->received(*this, bundle);
			}
			else
			{
				// raise default bundle received event
				EventSwitch::raiseEvent( new BundleEvent(bundle, BUNDLE_RECEIVED) );
				EventSwitch::raiseEvent( new RouteEvent(bundle, ROUTE_PROCESS_BUNDLE) );
			}
		}
	}
}
