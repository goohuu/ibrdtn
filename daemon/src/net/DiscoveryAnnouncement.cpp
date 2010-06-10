/*
 * DiscoveryAnnouncement.cpp
 *
 *  Created on: 11.09.2009
 *      Author: morgenro
 */

#include "net/DiscoveryAnnouncement.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BundleString.h"
#include <ibrcommon/Logger.h>
#include <netinet/in.h>
#include <typeinfo>

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		DiscoveryAnnouncement::DiscoveryAnnouncement(bool short_beacon, dtn::data::EID eid)
		 : _short_beacon(short_beacon), _flags(BEACON_NO_FLAGS), _canonical_eid(eid)
		{
			if (_short_beacon)
			{
				_flags = DiscoveryAnnouncement::BEACON_SHORT;
			}
		}

		DiscoveryAnnouncement::~DiscoveryAnnouncement()
		{
		}

		dtn::data::EID DiscoveryAnnouncement::getEID() const
		{
			return _canonical_eid;
		}

		const list<DiscoveryService> DiscoveryAnnouncement::getServices() const
		{
			return _services;
		}

		void DiscoveryAnnouncement::clearServices()
		{
			_services.clear();
		}

		const DiscoveryService& DiscoveryAnnouncement::getService(string name) const
		{
			list<DiscoveryService>::const_iterator iter = _services.begin();

			while (iter != _services.end())
			{
				iter++;
			}
		}

		void DiscoveryAnnouncement::addService(DiscoveryService service)
		{
			_services.push_back(service);
		}

		void DiscoveryAnnouncement::updateServices()
		{
			for (list<DiscoveryService>::iterator iter = _services.begin(); iter != _services.end(); iter++)
			{
				(*iter).update();
			}
		}

		std::ostream &operator<<(std::ostream &stream, const DiscoveryAnnouncement &announcement)
		{
			if (DiscoveryAnnouncement::BEACON_SHORT == announcement._flags)
			{
				stream << (unsigned char)DiscoveryAnnouncement::DISCO_VERSION_00 << announcement._flags;
				return stream;
			}

			dtn::data::BundleString eid(announcement._canonical_eid.getString());
			dtn::data::SDNV beacon_len;

			// determine the beacon length
			beacon_len += eid.getLength();

			// add service block length
			const list<DiscoveryService> &services = announcement._services;
			list<DiscoveryService>::const_iterator iter = services.begin();
			while (iter != services.end())
			{
				beacon_len += (*iter).getLength();
				iter++;
			}

			stream << (unsigned char)DiscoveryAnnouncement::DISCO_VERSION_00 << announcement._flags << beacon_len << eid;

			iter = services.begin();
			while (iter != services.end())
			{
				stream << (*iter);
				iter++;
			}

			return stream;
		}

		std::istream &operator>>(std::istream &stream, DiscoveryAnnouncement &announcement)
		{
			unsigned char version = stream.get();

			switch (version)
			{
			case DiscoveryAnnouncement::DISCO_VERSION_00:
			{
				IBRCOMMON_LOGGER_DEBUG(15) << "beacon version 1 received" << IBRCOMMON_LOGGER_ENDL;

				dtn::data::SDNV beacon_len;
				dtn::data::SDNV eid_len;

				stream >> announcement._flags;

				// catch a short beacon
				if (DiscoveryAnnouncement::BEACON_SHORT == announcement._flags)
				{
					announcement._canonical_eid = dtn::data::EID();
					return stream;
				}

				stream >> beacon_len; int remain = beacon_len.getValue();

				dtn::data::BundleString eid;
				stream >> eid; remain -= eid.getLength();
				announcement._canonical_eid = dtn::data::EID((std::string)eid);

				// get the services
				list<DiscoveryService> &services = announcement._services;

				// clear the services
				services.clear();

				while (remain > 0)
				{
					// decode the service blocks
					DiscoveryService service;
					stream >> service;
					services.push_back(service);
					remain -= service.getLength();
				}
				break;
			}

			case DiscoveryAnnouncement::DISCO_VERSION_01:
			{
				IBRCOMMON_LOGGER_DEBUG(15) << "beacon version 2 received" << IBRCOMMON_LOGGER_ENDL;

				stream >> announcement._flags;

				IBRCOMMON_LOGGER_DEBUG(25) << "beacon flags: " << hex << (int)announcement._flags << IBRCOMMON_LOGGER_ENDL;

				u_int16_t sn = 0;
				stream.read((char*)&sn, 2);

				// convert from network byte order
				u_int16_t sequencenumber = ntohs(sn);

				IBRCOMMON_LOGGER_DEBUG(25) << "beacon sequence number: " << sequencenumber << IBRCOMMON_LOGGER_ENDL;

				if (announcement._flags & DiscoveryAnnouncement::BEACON_CONTAINS_EID)
				{
					dtn::data::BundleString eid;
					stream >> eid;

					announcement._canonical_eid = dtn::data::EID((std::string)eid);

					IBRCOMMON_LOGGER_DEBUG(25) << "beacon eid: " << (std::string)eid << IBRCOMMON_LOGGER_ENDL;
				}

				if (announcement._flags & DiscoveryAnnouncement::BEACON_SERVICE_BLOCK)
				{
					// get the services
					list<DiscoveryService> &services = announcement._services;

					// read the number of services
					dtn::data::SDNV num_services;
					stream >> num_services;

					IBRCOMMON_LOGGER_DEBUG(25) << "beacon services (" << num_services.getValue() << "): " << IBRCOMMON_LOGGER_ENDL;

					// clear the services
					services.clear();

					for (int i = 0; i < num_services.getValue(); i++)
					{
						// decode the service blocks
						DiscoveryService service;
						stream >> service;
						services.push_back(service);

						IBRCOMMON_LOGGER_DEBUG(25) << "\t " << service.getName() << " [" << service.getParameters() << "]" << IBRCOMMON_LOGGER_ENDL;
					}
				}

				if (announcement._flags & DiscoveryAnnouncement::BEACON_BLOOMFILTER)
				{
					// TODO: read the bloomfilter
				}

				break;
			}

			default:
				IBRCOMMON_LOGGER_DEBUG(20) << "unknown beacon received" << IBRCOMMON_LOGGER_ENDL;

				// Error, throw Exception!
				throw InvalidProtocolException("The received data does not match the discovery protocol.");

				break;
			}

			return stream;
		}

		string DiscoveryAnnouncement::toString() const
		{
			stringstream ss;
			ss << "ANNOUNCE: " << _canonical_eid.getString();
			return ss.str();
		}
	}
}
