#ifndef LOWPANCONNECTION_H_
#define LOWPANCONNECTION_H_

#include "ibrcommon/Exceptions.h"
#include "lowpanstream.h"
#include "net/LOWPANConvergenceLayer.h"

#include "ibrdtn/data/BundleID.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class LOWPANConvergenceLayer;
		class lowpanstream;
		class LOWPANConnection : public ibrcommon::JoinableThread
		{
		public:
			LOWPANConnection(unsigned short address, LOWPANConvergenceLayer &cl);

			virtual ~LOWPANConnection();

			unsigned short address;

			lowpanstream& getStream();

			void run();

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

		protected:

		private:
			bool _running;
			lowpanstream *_stream;
		};
	}
}
#endif /*LOWPANCONNECTION_H_*/
