/*
 * DiscoveryAnnouncement.cpp
 *
 *  Created on: 11.09.2009
 *      Author: morgenro
 */

#include "net/DiscoveryAnnouncement.h"
#include <ibrdtn/data/Exceptions.h>
#include <ibrdtn/data/SDNV.h>
#include <ibrcommon/Logger.h>
#include <netinet/in.h>
#include <typeinfo>

using namespace dtn::data;

namespace dtn
{
	namespace net
	{
		DiscoveryAnnouncement::DiscoveryAnnouncement(const DiscoveryVersion version, dtn::data::EID eid)
		 : _version(version), _flags(BEACON_NO_FLAGS), _canonical_eid(eid)
		{
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
			for (std::list<DiscoveryService>::const_iterator iter = _services.begin(); iter != _services.end(); iter++)
			{
				if ((*iter).getName() == name)
				{
					return (*iter);
				}
			}

			throw dtn::MissingObjectException("No service found with tag " + name);
		}

		void DiscoveryAnnouncement::addService(DiscoveryService service)
		{
			_services.push_back(service);
		}

		void DiscoveryAnnouncement::setSequencenumber(u_int16_t sequence)
		{
			_sn = sequence;
		}

		std::ostream &operator<<(std::ostream &stream, const DiscoveryAnnouncement &announcement)
		{
			const list<DiscoveryService> &services = announcement._services;

			switch (announcement._version)
			{
				case DiscoveryAnnouncement::DISCO_VERSION_00:
				{
					if (services.empty())
					{
						unsigned char flags = DiscoveryAnnouncement::BEACON_SHORT | announcement._flags;
						stream << (unsigned char)DiscoveryAnnouncement::DISCO_VERSION_00 << flags;
						return stream;
					}

					dtn::data::BundleString eid(announcement._canonical_eid.getString());
					dtn::data::SDNV beacon_len;

					// determine the beacon length
					beacon_len += eid.getLength();

					// add service block length
					for (list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); iter++)
					{
						beacon_len += (*iter).getLength();
					}

					stream << (unsigned char)DiscoveryAnnouncement::DISCO_VERSION_00 << announcement._flags << beacon_len << eid;

					for (list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); iter++)
					{
						stream << (*iter);
					}
				}

				case DiscoveryAnnouncement::DISCO_VERSION_01:
				{
					unsigned char flags = 0;

					stream << (unsigned char)DiscoveryAnnouncement::DISCO_VERSION_01;

					if (announcement._canonical_eid != EID())
					{
						flags |= DiscoveryAnnouncement::BEACON_CONTAINS_EID;
					}

					if (!services.empty())
					{
						flags |= DiscoveryAnnouncement::BEACON_SERVICE_BLOCK;
					}

					stream << flags;

					// sequencenumber
					u_int16_t sn = htons(announcement._sn);
					stream.write( (char*)&sn, 2 );

					if ( flags && DiscoveryAnnouncement::BEACON_CONTAINS_EID )
					{
						dtn::data::BundleString eid(announcement._canonical_eid.getString());
						stream << eid;
					}

					if ( flags && DiscoveryAnnouncement::BEACON_SERVICE_BLOCK )
					{
						stream << dtn::data::SDNV(services.size());

						for (list<DiscoveryService>::const_iterator iter = services.begin(); iter != services.end(); iter++)
						{
							stream << (*iter);
						}
					}
				}
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

					for (unsigned int i = 0; i < num_services.getValue(); i++)
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
