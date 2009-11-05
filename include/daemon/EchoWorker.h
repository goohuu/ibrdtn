#ifndef ECHOWORKER_H_
#define ECHOWORKER_H_

#include "ibrdtn/default.h"
#include "core/AbstractWorker.h"

using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		/**
		 * This is a implementation of AbstractWorker and is comparable with
		 * a application. This application can send/receive bundles and implements
		 * a basic echo funcionality. If a bundle is received the payload of the
		 * bundle is copied and returned to the sender.
		 *
		 * The application suffix to the node eid is /echo.
		 */
		class EchoWorker : public AbstractWorker
		{
		public:
			EchoWorker();
			virtual ~EchoWorker() {};

			dtn::net::TransmitReport callbackBundleReceived(const Bundle &b);
		};
	}
}

#endif /*ECHOWORKER_H_*/
