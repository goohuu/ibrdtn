#include "daemon/EchoWorker.h"
#include "data/BundleFactory.h"
#include "core/ConvergenceLayer.h"
#include "data/PayloadBlockFactory.h"

using namespace dtn::core;
using namespace dtn::data;

namespace dtn
{
	namespace daemon
	{
		EchoWorker::EchoWorker() : AbstractWorker("/echo")
		{
			m_localuri = BundleCore::getInstance().getLocalURI();
		}

		TransmitReport EchoWorker::callbackBundleReceived(const Bundle &b)
		{
			const PayloadBlock *payload = dynamic_cast<PayloadBlock*>( b.getPayloadBlock() );

			// check if a payload block exists
			if ( payload != NULL )
			{
				// generate a echo
				BundleFactory &fac = BundleFactory::getInstance();
				Bundle *echo = fac.newBundle();
				PayloadBlock *block = PayloadBlockFactory::newPayloadBlock( payload->getPayload(), payload->getLength() );
				echo->appendBlock(block);

				// set destination and source
				echo->setDestination( b.getSource() );
				echo->setSource( m_localuri + getWorkerURI() );

				// send it
				transmit( *echo );
				delete echo;
			}

			return BUNDLE_ACCEPTED;
		}
	}
}
