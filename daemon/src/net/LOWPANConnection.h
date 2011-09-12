#ifndef LOWPANCONNECTION_H_
#define LOWPANCONNECTION_H_

#include "Component.h"
#include "ibrcommon/Exceptions.h"
#include <ibrcommon/net/vinterface.h>
#include <ibrcommon/net/lowpanstream.h>

#include "ibrdtn/data/BundleID.h"
#include "core/Node.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class LOWPANConvergenceLayer;
		class LOWPANConnection : public ibrcommon::JoinableThread
		{
		public:
			LOWPANConnection(unsigned int address, LOWPANConvergenceLayer &cl);

			virtual ~LOWPANConnection();

			unsigned int address;

//			ibrcommon::lowpanstream& getStream();

			void run();
#if 0
			class LOWPANConnectionSender : public ibrcommon::JoinableThread
			{
				public:
					LOWPANConnectionSender(ibrcommon::lowpanstream &stream);
					virtual ~LOWPANConnectionSender();

					void send(BundleID &id);
					void run();
				private:
					ibrcommon::lowpanstream &stream;
			};
#endif
		protected:

		private:
			bool _running;
//			ibrcommon::lowpanstream _stream;
		};
	}
}
#endif /*LOWPANCONNECTION_H_*/
