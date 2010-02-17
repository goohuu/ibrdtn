/*
 * StatusReportGenerator.cpp
 *
 *  Created on: 17.02.2010
 *      Author: morgenro
 */

#include "core/StatusReportGenerator.h"
#include "core/BundleEvent.h"
#include "routing/QueueBundleEvent.h"
#include "core/BundleCore.h"

namespace dtn
{
	namespace core
	{
		using namespace dtn::data;

		StatusReportGenerator::StatusReportGenerator()
		{
			bindEvent(BundleEvent::className);
		}

		StatusReportGenerator::~StatusReportGenerator()
		{
			unbindEvent(BundleEvent::className);
		}

		Bundle StatusReportGenerator::createStatusReport(const Bundle &b, StatusReportBlock::TYPE type, StatusReportBlock::REASON_CODE reason)
		{
			// create a new bundle
			Bundle bundle;

			// create a new statusreport block
			StatusReportBlock *report = new StatusReportBlock();

			// get the flags and set the status flag
			if (!(report->_status & type)) report->_status += type;

			// set the reason code
			if (!(report->_reasoncode & reason)) report->_reasoncode += reason;

			switch (type)
			{
				case StatusReportBlock::RECEIPT_OF_BUNDLE:
					report->_timeof_receipt.set();
				break;

				case StatusReportBlock::CUSTODY_ACCEPTANCE_OF_BUNDLE:
					report->_timeof_custodyaccept.set();
				break;

				case StatusReportBlock::FORWARDING_OF_BUNDLE:
					report->_timeof_forwarding.set();
				break;

				case StatusReportBlock::DELIVERY_OF_BUNDLE:
					report->_timeof_delivery.set();
				break;

				case StatusReportBlock::DELETION_OF_BUNDLE:
					report->_timeof_deletion.set();
				break;

				default:

				break;
			}

			// set source and destination
			bundle._source = dtn::core::BundleCore::local;
			bundle._destination = b._reportto;

			// set bundle parameter
			if (b._procflags & Bundle::FRAGMENT)
			{
				report->_fragment_offset = b._fragmentoffset;
				report->_fragment_length = b._appdatalength;

				if (!(report->_admfield & 1)) report->_admfield += 1;
			}

			report->_bundle_timestamp = b._timestamp;
			report->_bundle_sequence = b._sequencenumber;
			report->_source = b._source;

			// commit the data
			report->commit();

			// add the report to the bundle
			bundle.addBlock(report);

			return bundle;
		}

		void StatusReportGenerator::raiseEvent(const Event *evt)
		{
			const BundleEvent *bundleevent = dynamic_cast<const BundleEvent*>(evt);

			if (bundleevent != NULL)
			{
				const Bundle &b = bundleevent->getBundle();

				switch (bundleevent->getAction())
				{
				case BUNDLE_RECEIVED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_RECEPTION )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::RECEIPT_OF_BUNDLE, bundleevent->getReason());
						dtn::routing::QueueBundleEvent::raise(bundle);
					}
					break;
				case BUNDLE_DELETED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_DELETION )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::DELETION_OF_BUNDLE, bundleevent->getReason());
						dtn::routing::QueueBundleEvent::raise(bundle);
					}
					break;

				case BUNDLE_FORWARDED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_FORWARDING )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::FORWARDING_OF_BUNDLE, bundleevent->getReason());
						dtn::routing::QueueBundleEvent::raise(bundle);
					}
					break;

				case BUNDLE_DELIVERED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_BUNDLE_DELIVERY )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::DELIVERY_OF_BUNDLE, bundleevent->getReason());
						dtn::routing::QueueBundleEvent::raise(bundle);
					}
					break;

				case BUNDLE_CUSTODY_ACCEPTED:
					if ( b._procflags & Bundle::REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE )
					{
						Bundle bundle = createStatusReport(b, StatusReportBlock::CUSTODY_ACCEPTANCE_OF_BUNDLE, bundleevent->getReason());
						dtn::routing::QueueBundleEvent::raise(bundle);
					}
					break;
				}
			}
		}
	}
}
