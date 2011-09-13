#ifndef LOWPANCONNECTION_H_
#define LOWPANCONNECTION_H_

//#include "Component.h"
#include "ibrcommon/Exceptions.h"
//#include <ibrcommon/net/vinterface.h>
#include "lowpanstream.h"

#include "ibrdtn/data/BundleID.h"
//#include "core/Node.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class LOWPANConvergenceLayer;
		class LOWPANConnection : public ibrcommon::JoinableThread
		{
		public:
			//LOWPANConnection(unsigned short address, LOWPANConvergenceLayer &cl);
			LOWPANConnection(unsigned short address);

			virtual ~LOWPANConnection();

			unsigned short address;

//			lowpanstream& getStream();

			void run();
#if 0
			class LOWPANConnectionSender : public ibrcommon::JoinableThread
			{
				public:
					LOWPANConnectionSender(lowpanstream &stream);
					virtual ~LOWPANConnectionSender();

					void send(BundleID &id);
					void run();
				private:
					lowpanstream &stream;
			};
#endif
		protected:

		private:
			bool _running;
//			lowpanstream _stream;
		};
	}
}
#endif /*LOWPANCONNECTION_H_*/
