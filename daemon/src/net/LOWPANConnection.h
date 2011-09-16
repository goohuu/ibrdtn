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
		class LOWPANConnectionSender;
		class LOWPANConnection : public ibrcommon::JoinableThread
		{
		public:
			LOWPANConnection(unsigned short address, LOWPANConvergenceLayer &cl);

			virtual ~LOWPANConnection();

			unsigned short address;

			lowpanstream& getStream();

			void run();

			LOWPANConnectionSender *_sender;

		protected:

		private:
			bool _running;
			lowpanstream _stream;
		};

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
	}
}
#endif /*LOWPANCONNECTION_H_*/
