#ifndef LOWPANCONNECTION_H_
#define LOWPANCONNECTION_H_

#include "Component.h"
#include "ibrcommon/Exceptions.h"
#include <ibrcommon/net/vinterface.h>

#include "ibrdtn/data/BundleID.h"
#include "core/Node.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class LOWPANConnection : public ibrcommon::JoinableThread
		{
		public:
			LOWPANConnection(ibrcommon::vinterface net);

			virtual ~LOWPANConnection();

			unsigned int address;

			void run();

			void queue(const dtn::core::Node &n);

			LOWPANConnection& operator>>(dtn::data::Bundle&);

			virtual const std::string getName() const;
#if 0
			class LOWPANConnectionSender : public LOWPANConnection
			{
				public:
					send(BundleID &id);

					queue();
			};
#endif
		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();

		private:
			ibrcommon::vinterface _net;

			unsigned int m_maxmsgsize;

			ibrcommon::Mutex m_writelock;
			ibrcommon::Mutex m_readlock;

			bool _running;
		};
	}
}
#endif /*LOWPANCONNECTION_H_*/
