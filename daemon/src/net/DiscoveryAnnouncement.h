/*
 * DiscoveryAnnouncement.h
 *
 *  Created on: 09.09.2009
 *      Author: morgenro
 */

#ifndef DISCOVERYANNOUNCEMENT_H_
#define DISCOVERYANNOUNCEMENT_H_

#include "ibrdtn/config.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/EID.h"
#include "net/DiscoveryService.h"
#include <string>
#include <list>

namespace dtn
{
	namespace net
	{
		class DiscoveryAnnouncement
		{
			enum BEACON_FLAGS_V1
			{
				BEACON_NO_FLAGS = 0x00,
				BEACON_SHORT = 0x01
			};

			enum BEACON_FLAGS_V2
			{
				BEACON_CONTAINS_EID = 0x01,
				BEACON_SERVICE_BLOCK = 0x02,
				BEACON_BLOOMFILTER = 0x04
			};

		public:
			DiscoveryAnnouncement(bool short_beacon = false, dtn::data::EID eid = dtn::data::EID());
			virtual ~DiscoveryAnnouncement();

			dtn::data::EID getEID() const;

			const std::list<DiscoveryService> getServices() const;
			void clearServices();
			void addService(DiscoveryService service);
			const DiscoveryService& getService(string name) const;

			string toString() const;

			/**
			 * update all service blocks
			 */
			virtual void updateServices();

		private:
			enum DiscoveryVersion
			{
				DISCO_VERSION_00 = 0x01,
				DISCO_VERSION_01 = 0x02
			};

			friend std::ostream &operator<<(std::ostream &stream, const DiscoveryAnnouncement &announcement);
			friend std::istream &operator>>(std::istream &stream, DiscoveryAnnouncement &announcement);

			bool _short_beacon;
			unsigned char _flags;
			dtn::data::EID _canonical_eid;

			list<DiscoveryService> _services;
		};
	}
}

#endif /* DISCOVERYANNOUNCEMENT_H_ */
