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

			// Prüfen ob überhaupt ein PayloadBlock da ist.
			if ( payload != NULL )
			{
				// Echo generieren
				BundleFactory &fac = BundleFactory::getInstance();
				Bundle *echo = fac.newBundle();
				PayloadBlock *block = PayloadBlockFactory::newPayloadBlock( payload->getPayload(), payload->getLength() );
				echo->appendBlock(block);

				// Empfänger und Absender setzen
				echo->setDestination( b.getSource() );
				echo->setSource( m_localuri + getWorkerURI() );

				// Absenden
				transmit( *echo );
				delete echo;
			}

			return BUNDLE_ACCEPTED;
		}
	}
}
