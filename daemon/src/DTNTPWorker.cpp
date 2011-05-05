/*
 * DTNTPWorker.cpp
 *
 *  Created on: 05.05.2011
 *      Author: morgenro
 */

#include "DTNTPWorker.h"
#include "core/NodeEvent.h"
#include "core/BundleCore.h"
#include <ibrdtn/utils/Clock.h>
#include <ibrdtn/data/AgeBlock.h>
#include <ibrdtn/data/PayloadBlock.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrcommon/TimeMeasurement.h>
#include <ibrcommon/Logger.h>

namespace dtn
{
	namespace daemon
	{
		DTNTPWorker::DTNTPWorker()
		{
			AbstractWorker::initialize("/dtntp", true);
			bindEvent(dtn::core::NodeEvent::className);
		}

		DTNTPWorker::~DTNTPWorker()
		{
			unbindEvent(dtn::core::NodeEvent::className);
		}

		size_t DTNTPWorker::getTimestamp() const
		{
			struct timeval detail_time;
			gettimeofday(&detail_time, NULL);
			return (detail_time.tv_sec * 1000000) + detail_time.tv_usec;
		}

		void DTNTPWorker::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::core::NodeEvent &n = dynamic_cast<const dtn::core::NodeEvent&>(*evt);

				if (n.getAction() == dtn::core::NODE_AVAILABLE)
				{
					// send a time sync bundle
					dtn::data::Bundle b;

					// add a age block
					b.push_back<dtn::data::AgeBlock>();

					ibrcommon::BLOB::Reference ref = ibrcommon::StringBLOB::create();
					(*ref.iostream()) << dtn::data::SDNV(getTimestamp());
					b.push_back(ref);

					b._source = dtn::core::BundleCore::local + "/dtntp";
					b._destination = n.getNode().getEID() + "/dtntp";

					transmit(b);
				}
			} catch (const std::bad_cast&) { };
		}

		void DTNTPWorker::callbackBundleReceived(const Bundle &b)
		{
			try {
				// read payload block
				const dtn::data::PayloadBlock &p = b.getBlock<dtn::data::PayloadBlock>();
				dtn::data::SDNV timestamp;
				(*p.getBLOB().iostream()) >> timestamp;

				// read the ageblock of the bundle
				const dtn::data::AgeBlock &age = b.getBlock<dtn::data::AgeBlock>();
				size_t offset = getTimestamp() - (timestamp.getValue() + age.getAge());

				// print out offset to the local clock
				IBRCOMMON_LOGGER(info) << "DT-NTP bundle received; clock of " << b._source.getNodeEID() << " has a offset of " << offset << " us" << IBRCOMMON_LOGGER_ENDL;
			} catch (const ibrcommon::Exception&) { };
		}
	}
}
