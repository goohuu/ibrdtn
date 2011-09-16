#ifndef LOWPANCONNECTION_H_
#define LOWPANCONNECTION_H_

#include "ibrcommon/Exceptions.h"
#include "ibrcommon/thread/Queue.h"
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
		class LOWPANConnectionSender : public ibrcommon::JoinableThread
		{
			public:
				LOWPANConnectionSender(lowpanstream &stream);
				virtual ~LOWPANConnectionSender();

				void queue(const ConvergenceLayer::Job &job);
				void run();

			private:
				lowpanstream &stream;
				ibrcommon::Queue<ConvergenceLayer::Job> _queue;
		};
		class LOWPANConnection : public ibrcommon::JoinableThread
		{
		public:
			LOWPANConnection(unsigned short address, LOWPANConvergenceLayer &cl);

			virtual ~LOWPANConnection();

			unsigned short address;

			lowpanstream& getStream();

			void run();
			void setup();

			LOWPANConnectionSender _sender;

		protected:

		private:
			bool _running;
			lowpanstream _stream;
		};

	}
}
#endif /*LOWPANCONNECTION_H_*/
