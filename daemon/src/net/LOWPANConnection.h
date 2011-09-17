#ifndef LOWPANCONNECTION_H_
#define LOWPANCONNECTION_H_

#include "ibrcommon/thread/Queue.h"
#include "lowpanstream.h"
#include "net/LOWPANConvergenceLayer.h"

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		class lowpanstream;
		class LOWPANConnectionSender : public ibrcommon::JoinableThread
		{
			public:
				LOWPANConnectionSender(lowpanstream &_stream);
				virtual ~LOWPANConnectionSender();

				void queue(const ConvergenceLayer::Job &job);
				void run();

			private:
				lowpanstream &_stream;
				ibrcommon::Queue<ConvergenceLayer::Job> _queue;
		};

		class LOWPANConvergenceLayer;
		class LOWPANConnection : public ibrcommon::JoinableThread
		{
		public:
			LOWPANConnection(unsigned short _address, LOWPANConvergenceLayer &cl);

			virtual ~LOWPANConnection();

			unsigned short _address;

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
