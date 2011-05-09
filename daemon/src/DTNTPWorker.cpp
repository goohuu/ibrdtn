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
				_last_sync.origin_quality = 0.0;
				_last_sync.peer_quality = 1.0;
				_last_sync.peer_timestamp = _last_sync.origin_timestamp;
			}
			else
			{
				_last_sync.origin_quality = 0.0;
				_last_sync.peer_quality = 0.0;
			}

			// set current quality to last sync quality
			dtn::utils::Clock::quality = _last_sync.peer_quality;

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

		DTNTPWorker::TimeSyncMessage::TimeSyncMessage()
		 : type(TIMESYNC_REQUEST), origin_quality(dtn::utils::Clock::quality), peer_quality(0.0)
		{
			timerclear(&origin_timestamp);
			timerclear(&peer_timestamp);

			struct timezone tz;
			gettimeofday(&origin_timestamp, &tz);
		}

		DTNTPWorker::TimeSyncMessage::~TimeSyncMessage()
		{
		}

		std::ostream &operator<<(std::ostream &stream, const DTNTPWorker::TimeSyncMessage &obj)
		{
			std::stringstream ss;

			stream << (char)obj.type;

			ss.clear(); ss.str(""); ss << obj.origin_quality;
			stream << dtn::data::BundleString(ss.str());

			stream << dtn::data::SDNV(obj.origin_timestamp.tv_sec);
			stream << dtn::data::SDNV(obj.origin_timestamp.tv_usec);

			ss.clear(); ss.str(""); ss << obj.peer_quality;
			stream << dtn::data::BundleString(ss.str());

			stream << dtn::data::SDNV(obj.peer_timestamp.tv_sec);
			stream << dtn::data::SDNV(obj.peer_timestamp.tv_usec);

			return stream;
		}

		std::istream &operator>>(std::istream &stream, DTNTPWorker::TimeSyncMessage &obj)
		{
			char type = 0;
			std::stringstream ss;
			dtn::data::BundleString bs;
			dtn::data::SDNV sdnv;

			stream >> type; obj.type = DTNTPWorker::TimeSyncMessage::MSG_TYPE(type);

			stream >> bs; ss.clear(); ss.str((std::string&)bs);
			ss >> obj.origin_quality;

			stream >> sdnv; obj.origin_timestamp.tv_sec = sdnv.getValue();
			stream >> sdnv; obj.origin_timestamp.tv_usec = sdnv.getValue();

			stream >> bs; ss.clear(); ss.str((std::string&)bs);
			ss >> obj.peer_quality;

			stream >> sdnv; obj.peer_timestamp.tv_sec = sdnv.getValue();
			stream >> sdnv; obj.peer_timestamp.tv_usec = sdnv.getValue();

			return stream;
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
							_sync_age++;

							// adjust own quality of time
							dtn::utils::Clock::quality = _last_sync.peer_quality * (1 / ::pow(_sigma, _sync_age) );

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

						// create a new timesync request
						TimeSyncMessage msg;

						// write the message
						(*stream) << msg;
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

//		void DTNTPWorker::shared_sync(const TimeBeacon &beacon)
//		{
//			// do not sync if we are a reference
//			if (_conf.hasReference()) return;
//
//			// adjust own quality of time
//			TimeBeacon current; get_time(current);
//			long int tdiff = current.sec - _last_sync.sec;
//
//			ibrcommon::MutexLock l(_sync_lock);
//
//			// if we have no time, take it
//			if (_last_sync.quality == 0)
//			{
//				dtn::utils::Clock::quality = beacon.quality * _epsilon;
//			}
//			// if our last sync is older than one second...
//			else if (tdiff > 0)
//			{
//				// sync our clock
//				double ext_faktor = beacon.quality / (beacon.quality + dtn::utils::Clock::quality);
//				double int_faktor = dtn::utils::Clock::quality / (beacon.quality + dtn::utils::Clock::quality);
//
//				// set the new time values
//				set_time( 	(beacon.sec * ext_faktor) + (current.sec * int_faktor),
//							(beacon.usec * ext_faktor) + (current.usec * int_faktor)
//						);
//			}
//			else
//			{
//				return;
//			}
//		}

		void DTNTPWorker::sync(const TimeSyncMessage &msg, struct timeval &tv)
		{
			// do not sync if we are a reference
			if (_conf.hasReference()) return;

			ibrcommon::MutexLock l(_sync_lock);

			// if the received quality of time is worse than ours, ignore it
			if (dtn::utils::Clock::quality >= msg.peer_quality) return;

			// the values are better, adapt them
			dtn::utils::Clock::quality = msg.peer_quality * _epsilon;

			// set the local clock to the new timestamp
			::settimeofday(&tv, NULL);

			IBRCOMMON_LOGGER(info) << "time adjusted to " << msg.peer_timestamp.tv_sec << "." << msg.peer_timestamp.tv_usec << "; quality: " << dtn::utils::Clock::quality << IBRCOMMON_LOGGER_ENDL;

			// remember the last sync
			_last_sync = msg;
		}

		void DTNTPWorker::callbackBundleReceived(const Bundle &b)
		{
			// do not sync with ourselves
			if (b._source.getNodeEID() == dtn::core::BundleCore::local.getNodeEID()) return;

			try {
				// read payload block
				const dtn::data::PayloadBlock &p = b.getBlock<dtn::data::PayloadBlock>();

				// read the type of the message
				char type = 0; (*p.getBLOB().iostream()).get(type);

				switch (type)
				{
					case TimeSyncMessage::TIMESYNC_REQUEST:
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

							// read the timesync message
							TimeSyncMessage msg;
							(*stream) >> msg;

							// clear the payload
							stream.clear();

							// fill in the own values
							msg.type = TimeSyncMessage::TIMESYNC_RESPONSE;
							msg.peer_quality = dtn::utils::Clock::quality;
							struct timezone tz;
							gettimeofday(&msg.peer_timestamp, &tz);

							// write the response
							(*stream) << msg;
						}

						// add a second age block
						response.push_front<dtn::data::AgeBlock>();

						// send the response
						transmit(response);
						break;
					}

					case TimeSyncMessage::TIMESYNC_RESPONSE:
					{
						// read the ageblock of the bundle
						const std::list<const dtn::data::AgeBlock*> ageblocks = b.getBlocks<dtn::data::AgeBlock>();
						const dtn::data::AgeBlock &peer_age = (*ageblocks.front());
						const dtn::data::AgeBlock &origin_age = (*ageblocks.back());

						timeval tv_age; timerclear(&tv_age);
						tv_age.tv_usec = origin_age.getMicroseconds();

						ibrcommon::BLOB::Reference ref = p.getBLOB();
						ibrcommon::BLOB::iostream stream = ref.iostream();

						TimeSyncMessage msg; (*stream) >> msg;

						timeval tv_local, rtt;
						struct timezone tz;
						gettimeofday(&tv_local, &tz);

						// get the RTT
						timersub(&tv_local, &msg.origin_timestamp, &rtt);

						// get the propagation delay
						timeval prop_delay;
						timersub(&rtt, &tv_age, &prop_delay);

						// half the prop delay
						prop_delay.tv_sec /= 2;
						prop_delay.tv_usec /= 2;

						timeval sync_delay;
						timerclear(&sync_delay);
						sync_delay.tv_usec = peer_age.getMicroseconds() + prop_delay.tv_usec;

						timeval peer_timestamp;
						timeradd(&msg.peer_timestamp, &sync_delay, &peer_timestamp);

						timeval offset;
						timersub(&tv_local, &peer_timestamp, &offset);

						// print out offset to the local clock
						IBRCOMMON_LOGGER(info) << "DT-NTP bundle received; rtt = " << rtt.tv_sec << "s " << rtt.tv_usec << "us; prop. delay = " << prop_delay.tv_sec << "s " << prop_delay.tv_usec << "us; clock of " << b._source.getNodeEID() << " has a offset of " << offset.tv_sec << "s " << offset.tv_usec << "us" << IBRCOMMON_LOGGER_ENDL;

						// sync to this time message
						sync(msg, peer_timestamp);

						break;
					}
				}
			} catch (const ibrcommon::Exception&) { };
		}
	}
}
