#ifndef LOWPANCONVERGENCELAYER_H_
#define LOWPANCONVERGENCELAYER_H_

#include "Component.h"
#include "net/ConvergenceLayer.h"
#include "ibrcommon/Exceptions.h"
#include "net/DiscoveryServiceProvider.h"
#include "ibrcommon/net/NetInterface.h"
#include "ibrcommon/net/lowpansocket.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		/**
		 * This class implement a ConvergenceLayer for LOWPAN.
		 */
		class LOWPANConvergenceLayer : public ConvergenceLayer, public dtn::daemon::IndependentComponent, public DiscoveryServiceProvider
		{
		public:
			LOWPANConvergenceLayer(ibrcommon::NetInterface net, int panid, bool broadcast = false, unsigned int mtu = 127);

			virtual ~LOWPANConvergenceLayer();

			virtual void update(std::string &name, std::string &data);
			virtual bool onInterface(const ibrcommon::NetInterface &net) const;

			dtn::core::Node::Protocol getDiscoveryProtocol() const;

			void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

			LOWPANConvergenceLayer& operator>>(dtn::data::Bundle&);

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

		private:
			ibrcommon::lowpansocket *_socket;

			ibrcommon::NetInterface _net;
			int _panid;
			int m_socket;

			static const int DEFAULT_PANID;

			unsigned int m_maxmsgsize;

			ibrcommon::Mutex m_writelock;
			ibrcommon::Mutex m_readlock;

			bool _running;
		};
	}
}
#endif /*LOWPANCONVERGENCELAYER_H_*/
