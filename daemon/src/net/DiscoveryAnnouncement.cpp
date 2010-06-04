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

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		DiscoveryAnnouncement::DiscoveryAnnouncement(bool short_beacon, dtn::data::EID eid)
		 : _short_beacon(short_beacon), _flags(NO_FLAGS), _canonical_eid(eid)
		{
			if (_short_beacon)
			{
				_flags = DiscoveryAnnouncement::SHORT_BEACON;
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
			if (DiscoveryAnnouncement::SHORT_BEACON == announcement._flags)
			{
				stream << announcement.VERSION << announcement._flags;
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

			stream << announcement.VERSION << announcement._flags << beacon_len << eid;

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
			dtn::data::SDNV beacon_len;
			dtn::data::SDNV eid_len;
			unsigned char version;

			stream >> version;

			if (version != DiscoveryAnnouncement::VERSION)
			{
				// Error, throw Exception!
				throw InvalidProtocolException("The received data does not match the discovery protocol.");
			}

			stream >> announcement._flags;

			// catch a short beacon
			if (DiscoveryAnnouncement::SHORT_BEACON == announcement._flags)
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
