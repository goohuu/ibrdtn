#include "core/DummyConvergenceLayer.h"

namespace dtn
{
	namespace core
	{
		DummyConvergenceLayer::DummyConvergenceLayer(string name)
			: m_name(name)
		{
		}

		DummyConvergenceLayer::~DummyConvergenceLayer()
		{
		}

		TransmitReport DummyConvergenceLayer::transmit(const Bundle &b)
		{
			return TRANSMIT_SUCCESSFUL;
		}

		TransmitReport DummyConvergenceLayer::transmit(const Bundle &b, const Node &node)
		{
			return TRANSMIT_SUCCESSFUL;
		}

		string DummyConvergenceLayer::getName()
		{
			return m_name;
		}
	}
}
