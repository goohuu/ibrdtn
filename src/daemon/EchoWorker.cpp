#include "daemon/EchoWorker.h"
#include "net/ConvergenceLayer.h"
#include "ibrdtn/utils/Utils.h"

using namespace dtn::core;
using namespace dtn::data;

namespace dtn
{
	namespace daemon
	{
		EchoWorker::EchoWorker()
		{
			AbstractWorker::initialize("/echo", true);
		}

		dtn::net::TransmitReport EchoWorker::callbackBundleReceived(const Bundle &b)
		{
			PayloadBlock *payload = utils::Utils::getPayloadBlock( b );

			// check if a payload block exists
			if ( payload != NULL )
			{
				// generate a echo
				Bundle echo;

				// make a copy of the payload block
				PayloadBlock *payload_copy = new PayloadBlock(payload->getBLOB());

				// append to the bundle
				echo.addBlock(payload_copy);

				// set destination and source
				echo._destination = b._source;
				echo._source = getWorkerURI();

#ifdef DO_DEBUG_OUTPUT
				cout << "echo request received, replying!" << endl;
#endif

				// send it
				transmit( echo );
			}

			return dtn::net::BUNDLE_ACCEPTED;
		}
	}
}
