#include "core/ConvergenceLayer.h"
#include "core/BundleReceiver.h"

namespace dtn
{
	namespace core
	{
		void ConvergenceLayer::setBundleReceiver(BundleReceiver *receiver)
		{
			m_receiver = receiver;
		}

		void ConvergenceLayer::eventBundleReceived(Bundle &bundle)
		{
			if (m_receiver != NULL)
			{
				m_receiver->received(*this, bundle);
			}
		}
	}
}
