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
#include <iostream>

using namespace dtn::data;

namespace dtn
{
	namespace daemon
	{
		void TestApplication::reportIt()
		{
			BundleFactory &fac = BundleFactory::getInstance();
			Bundle *out = fac.newBundle();
			BundleCore &core = BundleCore::getInstance();

			string data = "Hello World!";

			PayloadBlock *payload = PayloadBlockFactory::newPayloadBlock((unsigned char*)data.c_str(), data.length());

			// set destination and source
			out->setDestination( m_destination );
			out->setSource( core.getLocalURI() + getWorkerURI() );
			out->setReportTo( core.getLocalURI() + getWorkerURI() );
			out->setInteger( LIFETIME, 200 );

			// custody required
			PrimaryFlags flags = out->getPrimaryFlags();
			flags.setCustodyRequested(true);
//			flags.setFlag(REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE, true);
			flags.setFlag(REQUEST_REPORT_OF_BUNDLE_DELIVERY, true);
			out->setPrimaryFlags( flags );

			// add payloadblock to bundle
			out->appendBlock(payload);

			// send the bundle
			transmit(*out);
			delete out;
		}

		void TestApplication::tick()
		{
			unsigned int curtime = BundleFactory::getDTNTime();

			if (m_dtntime <= curtime)
			{
				// get active every 5 seconds
				m_dtntime = curtime + 5;

				reportIt();
			}

			usleep(10);
		}

		TransmitReport TestApplication::callbackBundleReceived(const Bundle &b)
		{
			cout << "got it!" << endl;
			return BUNDLE_ACCEPTED;
		}
	}
}
