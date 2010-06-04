#include "EchoWorker.h"
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

		void EchoWorker::callbackBundleReceived(const Bundle &b)
		{
			try {
				const PayloadBlock &payload = b.getBlock<PayloadBlock>();

				// generate a echo
				Bundle echo;

				// make a copy of the payload block
				ibrcommon::BLOB::Reference ref = payload.getBLOB();
				PayloadBlock &payload_copy = echo.push_back(ref);

				// set destination and source
				echo._destination = b._source;
				echo._source = getWorkerURI();

#ifdef DO_DEBUG_OUTPUT
				cout << "echo request received, replying!" << endl;
#endif

				// send it
				transmit( echo );
			} catch (dtn::data::Bundle::NoSuchBlockFoundException ex) {

			}
		}
	}
}
