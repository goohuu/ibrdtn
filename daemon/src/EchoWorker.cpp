#include "EchoWorker.h"
#include "net/ConvergenceLayer.h"
#include "ibrdtn/utils/Utils.h"
#include <ibrcommon/Logger.h>

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
				IBRCOMMON_LOGGER_DEBUG(5) << "echo request received, replying!" << IBRCOMMON_LOGGER_ENDL;
#endif

				// send it
				transmit( echo );
			} catch (dtn::data::Bundle::NoSuchBlockFoundException ex) {

			}
		}
	}
}
