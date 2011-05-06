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

					// add an age block
					b.push_back<dtn::data::AgeBlock>();

					ibrcommon::BLOB::Reference ref = ibrcommon::StringBLOB::create();

					// create the payload of the message
					{
						ibrcommon::BLOB::iostream stream = ref.iostream();
						(*stream) << (char)MEASUREMENT_REQUEST;
						(*stream) << dtn::data::SDNV(getTimestamp());
					}

					// add the payload to the message
					b.push_back(ref);

					// set the source and destination
					b._source = dtn::core::BundleCore::local + "/dtntp";
					b._destination = n.getNode().getEID() + "/dtntp";

					// set the the destination as singleton receiver
					b.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);

					// set the lifetime of the bundle to 60 seconds
					b._lifetime = 60;

					transmit(b);
				}
			} catch (const std::bad_cast&) { };
		}

		void DTNTPWorker::callbackBundleReceived(const Bundle &b)
		{
			try {
				// read payload block
				const dtn::data::PayloadBlock &p = b.getBlock<dtn::data::PayloadBlock>();

				// read the type of the message
				char type = 0; (*p.getBLOB().iostream()).get(type);

				switch (type)
				{
					case MEASUREMENT_REQUEST:
					{
						dtn::data::Bundle response = b;
						response.relabel();

						// set the lifetime of the bundle to 60 seconds
						response._lifetime = 60;

						// switch the source and destination
						response._source = b._destination;
						response._destination = b._source;

						// set the the destination as singleton receiver
						response.set(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON, true);

						// modify the payload - locked
						{
							ibrcommon::BLOB::Reference ref = p.getBLOB();
							ibrcommon::BLOB::iostream stream = ref.iostream();

							// read the origin timestamp
							dtn::data::SDNV otime;
							(*stream) >> type;
							(*stream) >> otime;

							stream.clear();
							(*stream) << (char)MEASUREMENT_RESPONSE << otime << dtn::data::SDNV(getTimestamp());
						}

						// add a second age block
						response.push_front<dtn::data::AgeBlock>();

						// send the response
						transmit(response);
						break;
					}

					case MEASUREMENT_RESPONSE:
					{
						dtn::data::SDNV origin_timestamp, peer_timestamp;
						ibrcommon::BLOB::Reference ref = p.getBLOB();
						ibrcommon::BLOB::iostream stream = ref.iostream();
						(*stream).seekg(1);
						(*stream) >> origin_timestamp;
						(*stream) >> peer_timestamp;

						// read the ageblock of the bundle
						const std::list<const dtn::data::AgeBlock*> ageblocks = b.getBlocks<dtn::data::AgeBlock>();
						const dtn::data::AgeBlock &peer_age = (*ageblocks.front());
						const dtn::data::AgeBlock &origin_age = (*ageblocks.back());

						size_t local_timestamp = getTimestamp();

						// get the RTT
						size_t rtt = local_timestamp - origin_timestamp.getValue();

						// get the propagation delay
						size_t prop_delay = rtt - origin_age.getAge();

						size_t fixed_peer_timestamp = peer_timestamp.getValue() + peer_age.getAge() + (prop_delay/2);
						ssize_t offset = local_timestamp - fixed_peer_timestamp;

						// print out offset to the local clock
						IBRCOMMON_LOGGER(info) << "DT-NTP bundle received; rtt = " << rtt << "; prop. delay = " << prop_delay << "; clock of " << b._source.getNodeEID() << " has a offset of " << offset << " us" << IBRCOMMON_LOGGER_ENDL;
						break;
					}
				}
			} catch (const ibrcommon::Exception&) { };
		}
	}
}
