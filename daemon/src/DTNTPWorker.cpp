/*
 * DTNTPWorker.cpp
 *
 *  Created on: 05.05.2011
 *      Author: morgenro
 */

#include "DTNTPWorker.h"
#include "core/NodeEvent.h"
#include "core/BundleCore.h"
#include "core/TimeEvent.h"
#include <ibrdtn/utils/Clock.h>
#include <ibrdtn/data/AgeBlock.h>
#include <ibrdtn/data/PayloadBlock.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrcommon/TimeMeasurement.h>
#include <ibrcommon/Logger.h>

#include <sys/time.h>

namespace dtn
{
	namespace daemon
	{
		DTNTPWorker::DTNTPWorker()
		 : _conf(dtn::daemon::Configuration::getInstance().getTimeSync()), _sigma(1.001), _epsilon(0.99)
		{
			AbstractWorker::initialize("/dtntp", true);

			// the quality of a received time is at least 1 second worth.
			_epsilon = 1 / _sigma;

			if (_conf.hasReference())
			{
				get_time(_last_sync);
				_last_sync.quality = 1.0;
			}
			else
			{
				_last_sync.quality = 0;
			}

			// set current quality to last sync quality
			dtn::utils::Clock::quality = _last_sync.quality;

			if (_conf.syncOnDiscovery())
			{
				bindEvent(dtn::core::NodeEvent::className);
			}

			bindEvent(dtn::core::TimeEvent::className);

			// debug quality of time
			IBRCOMMON_LOGGER_DEBUG(10) << "quality of time is " << dtn::utils::Clock::quality << IBRCOMMON_LOGGER_ENDL;
		}

		DTNTPWorker::~DTNTPWorker()
		{
			unbindEvent(dtn::core::NodeEvent::className);
			unbindEvent(dtn::core::TimeEvent::className);
		}

		DTNTPWorker::TimeBeacon::TimeBeacon()
		 : nodeid(""), sec(0), usec(0), quality(0.0)
		{
		}

		DTNTPWorker::TimeBeacon::~TimeBeacon()
		{
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
				const dtn::core::TimeEvent &t = dynamic_cast<const dtn::core::TimeEvent&>(*evt);

				if (t.getAction() == dtn::core::TIME_SECOND_TICK)
				{
					ibrcommon::MutexLock l(_sync_lock);

					if ((_conf.getQualityOfTimeTick() > 0) && !_conf.hasReference())
					{
						/**
						 * decrease the quality of time each x tics
						 */
						if ((_qot_current_tic % _conf.getQualityOfTimeTick()) == 0)
						{
							// get current time values
							TimeBeacon current; get_time(current);
							long int tdiff = current.sec - _last_sync.sec;

							// adjust own quality of time
							dtn::utils::Clock::quality = _last_sync.quality * (1 / ::pow(_sigma, tdiff) );

							// debug quality of time
							IBRCOMMON_LOGGER_DEBUG(25) << "new quality of time is " << dtn::utils::Clock::quality << IBRCOMMON_LOGGER_ENDL;

							// reset the tick counter
							_qot_current_tic = 0;
						}
						else
						{
							// increment the tick counter
							_qot_current_tic++;
						}
					}
					else
					{
						/**
						 * evaluate the current local time
						 */
						if (dtn::utils::Clock::quality == 0)
						{
							if (dtn::utils::Clock::getTime() > 0)
							{
								dtn::utils::Clock::quality = 1;
								IBRCOMMON_LOGGER(warning) << "The local clock seems to be okay again. Expiration enabled." << IBRCOMMON_LOGGER_ENDL;
							}
						}
					}
				}
			} catch (const std::bad_cast&) { };

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

						// write the type
						(*stream) << (char)MEASUREMENT_REQUEST;

						// write the current timestamp
						(*stream) << dtn::data::SDNV(getTimestamp());

						// write the quality of time
						std::stringstream ss; ss << (float)dtn::utils::Clock::quality;
						(*stream) << dtn::data::BundleString(ss.str());
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

		void DTNTPWorker::set_time(size_t sec, size_t usec)
		{
			struct timeval tv;
			tv.tv_sec = sec;
			tv.tv_usec = usec;
			::settimeofday(&tv, NULL);
		}

		void DTNTPWorker::get_time(TimeBeacon &beacon)
		{
			struct timeval tv;
			struct timezone tz;
			gettimeofday(&tv, &tz);

			beacon.nodeid = dtn::core::BundleCore::local;
			beacon.sec = tv.tv_sec;
			beacon.usec = tv.tv_usec;
			beacon.quality = dtn::utils::Clock::quality;
		}

		void DTNTPWorker::shared_sync(const TimeBeacon &beacon)
		{
			// do not sync if we are a reference
			if (_conf.hasReference()) return;

			// adjust own quality of time
			TimeBeacon current; get_time(current);
			long int tdiff = current.sec - _last_sync.sec;

			ibrcommon::MutexLock l(_sync_lock);

			// if we have no time, take it
			if (_last_sync.quality == 0)
			{
				dtn::utils::Clock::quality = beacon.quality * _epsilon;
			}
			// if our last sync is older than one second...
			else if (tdiff > 0)
			{
				// sync our clock
				double ext_faktor = beacon.quality / (beacon.quality + dtn::utils::Clock::quality);
				double int_faktor = dtn::utils::Clock::quality / (beacon.quality + dtn::utils::Clock::quality);

				// set the new time values
				set_time( 	(beacon.sec * ext_faktor) + (current.sec * int_faktor),
							(beacon.usec * ext_faktor) + (current.usec * int_faktor)
						);
			}
			else
			{
				return;
			}
		}

		void DTNTPWorker::sync(const TimeBeacon &beacon)
		{
			// do not sync if we are a reference
			if (_conf.hasReference()) return;

			// do not sync with ourselves
			if (beacon.nodeid == dtn::core::BundleCore::local) return;

			ibrcommon::MutexLock l(_sync_lock);

			// if the received quality of time is worse than ours, ignore it
			if (dtn::utils::Clock::quality >= beacon.quality) return;

			// the values are better, adapt them
			dtn::utils::Clock::quality = beacon.quality * _epsilon;
			set_time( beacon.sec, beacon.usec );

			IBRCOMMON_LOGGER(info) << "time adjusted to " << beacon.sec << "." << beacon.usec << "; quality: " << dtn::utils::Clock::quality << IBRCOMMON_LOGGER_ENDL;

			// remember the last sync
			_last_sync = beacon;
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
							dtn::data::BundleString oqot;
							(*stream) >> type;
							(*stream) >> otime;
							(*stream) >> oqot;

							stream.clear();
							(*stream) << (char)MEASUREMENT_RESPONSE << otime << oqot << dtn::data::SDNV(getTimestamp());

							// write the quality of time
							std::stringstream ss; ss << (float)dtn::utils::Clock::quality;
							(*stream) << dtn::data::BundleString(ss.str());
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
						dtn::data::BundleString origin_qot, peer_qot;

						ibrcommon::BLOB::Reference ref = p.getBLOB();
						ibrcommon::BLOB::iostream stream = ref.iostream();
						(*stream).seekg(1);
						(*stream) >> origin_timestamp;
						(*stream) >> origin_qot;
						(*stream) >> peer_timestamp;
						(*stream) >> peer_qot;

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

						// convert to a time beacon
						struct timezone tz;
						timeval tv_offset, tv_local, tv_res;
						timerclear(&tv_offset);
						timerclear(&tv_local);

						gettimeofday(&tv_local, &tz);

						if (offset > 0)
						{
							// get seconds
							tv_offset.tv_sec = offset / 1000000;
							tv_offset.tv_usec = offset % 1000000;

							// add offset to current time
							timeradd(&tv_local, &tv_offset, &tv_res);
						}
						else if (offset < 0)
						{
							// turn offset into a positive value
							offset *= -1;

							// get seconds
							tv_offset.tv_sec = offset / 1000000;
							tv_offset.tv_usec = offset % 1000000;

							// add offset to current time
							timersub(&tv_local, &tv_offset, &tv_res);
						}

						TimeBeacon tb;
						tb.sec = tv_res.tv_sec;
						tb.usec = tv_res.tv_usec;
						tb.nodeid = b._source.getNodeEID();

						std::stringstream ss; ss << std::string(peer_qot); float qot = 0.0; ss >> qot;
						tb.quality = qot;

						// sync to this beacon
						sync(tb);

						break;
					}
				}
			} catch (const ibrcommon::Exception&) { };
		}
	}
}
