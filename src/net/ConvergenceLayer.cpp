#include "net/ConvergenceLayer.h"
#include "net/BundleReceiver.h"

namespace dtn
{
	namespace net
	{
		void ConvergenceLayer::setBundleReceiver(BundleReceiver *receiver)
		{
			m_receiver = receiver;
		}

		void ConvergenceLayer::eventBundleReceived(const Bundle &bundle)
		{
			if (m_receiver != NULL)
			{
				m_receiver->received(EID(), bundle);
			}
		}
	}
}
