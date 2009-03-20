#include "core/MultiplexConvergenceLayer.h"
#include <iostream>

namespace dtn
{
	namespace core
	{
		MultiplexConvergenceLayer::MultiplexConvergenceLayer()
		{
			m_clayers.clear();
		}

		MultiplexConvergenceLayer::~MultiplexConvergenceLayer()
		{
			list<ConvergenceLayer*>::iterator iter = m_clayers.begin();

			while (iter != m_clayers.end())
			{
				ConvergenceLayer *cl = (*iter);
				Service *s = dynamic_cast<Service*>(cl);
				if (s != NULL) s->abort();
				delete cl;
				iter++;
			}
		}

		TransmitReport MultiplexConvergenceLayer::transmit(const Bundle &b)
		{
			if (m_clayers.size() == 0) return UNKNOWN;
			ConvergenceLayer *cl = m_clayers.front();
			return cl->transmit(b);
		}

		TransmitReport MultiplexConvergenceLayer::transmit(const Bundle &b, const Node &node)
		{
			ConvergenceLayer *cl = node.getConvergenceLayer();
			if (cl == NULL) return transmit(b);
			return cl->transmit(b, node);
		}

		void MultiplexConvergenceLayer::received(const ConvergenceLayer &cl, const dtn::data::Bundle &b)
		{
			dtn::utils::MutexLock l(m_receivelock);
			ConvergenceLayer::eventBundleReceived(b);
		}

		void MultiplexConvergenceLayer::initialize()
		{
			list<ConvergenceLayer*>::iterator iter = m_clayers.begin();

			while (iter != m_clayers.end())
			{
				Service *s = dynamic_cast<Service*>( (*iter) );
				if (s != NULL)
				{
					if (!s->isRunning())
					{
						s->start();
					}
				}
				iter++;
			}
		}

		void MultiplexConvergenceLayer::terminate()
		{
			list<ConvergenceLayer*>::iterator iter = m_clayers.begin();

			while (iter != m_clayers.end())
			{
				Service *s = dynamic_cast<Service*>( (*iter) );
				if (s != NULL)
				{
					if (s->isRunning())
					{
						s->abort();
					}
				}
				iter++;
			}
		}

		void MultiplexConvergenceLayer::add(ConvergenceLayer *cl)
		{
			cl->setBundleReceiver(this);

			// ConvergenceLayer der Liste hinzufügen
			m_clayers.push_back(cl);
		}
	}
}
