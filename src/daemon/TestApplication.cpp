/*
 * TestApplication.cpp
 *
 *  Created on: 06.03.2009
 *      Author: morgenro
 */

#include "daemon/TestApplication.h"
#include "data/BundleFactory.h"
#include "data/PayloadBlock.h"
#include "data/PayloadBlockFactory.h"

using namespace dtn::data;

namespace dtn
{
	namespace daemon
	{
		void TestApplication::reportIt()
		{
			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *out = fac.newBundle();

			string data = "Hello World!";

			PayloadBlock *payload = PayloadBlockFactory::newPayloadBlock((unsigned char*)data.c_str(), data.length());

			// Setze den Empfänger und den Absender ein
			out->setDestination( m_destination );
			out->setSource( getCore().getLocalURI() + getWorkerURI() );
			out->setReportTo( getCore().getLocalURI() + getWorkerURI() );
			out->setInteger( LIFETIME, 200 );

			// Custody erforderlich!
			PrimaryFlags flags = out->getPrimaryFlags();
			flags.setCustodyRequested(true);
			flags.setFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE, true);
			flags.setFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY, true);
			out->setPrimaryFlags( flags );

			// Versende das Bundle
			TransmitReport status = getCore().transmit(out);

			// Status auswerten
			switch ( status )
			{
				case NO_ROUTE_FOUND:
					delete out;
				break;

				default:

				break;
			}
		}

		void TestApplication::tick()
		{
			unsigned int curtime = BundleFactory::getDTNTime();

			if (m_dtntime <= curtime)
			{
				// Alle 5 Sekunden aktiv werden
				m_dtntime = curtime + 5;

				reportIt();
			}

			usleep(10);
		}

		TransmitReport TestApplication::callbackBundleReceived(const Bundle &b)
		{
			return BUNDLE_ACCEPTED;
		}
	}
}
