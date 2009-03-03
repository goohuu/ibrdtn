#ifndef ECHOWORKER_H_
#define ECHOWORKER_H_

#include "core/AbstractWorker.h"

using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		class EchoWorker : public AbstractWorker
		{
		public:
			EchoWorker(BundleCore *core);
			~EchoWorker() {};

			TransmitReport callbackBundleReceived(Bundle *b);
		private:
			string m_localuri;
		};
	}
}

#endif /*ECHOWORKER_H_*/
