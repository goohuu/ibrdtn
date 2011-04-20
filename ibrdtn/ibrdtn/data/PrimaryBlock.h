/*
 * PrimaryBlock.h
 *
 *  Created on: 26.05.2010
 *      Author: morgenro
 */

#ifndef PRIMARYBLOCK_H_
#define PRIMARYBLOCK_H_

#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/Dictionary.h"
#include "ibrdtn/data/Serializer.h"
#include <ibrcommon/thread/Mutex.h>
#include <string>
#include <iostream>

namespace dtn
{
	namespace data
	{
		static const unsigned char BUNDLE_VERSION = 0x06;

		class PrimaryBlock
		{
			friend class DefaultSerializer;
			friend class DefaultDeserializer;

		public:
			enum FLAGS
			{
				FRAGMENT = 1 << 0x00,
				APPDATA_IS_ADMRECORD = 1 << 0x01,
				DONT_FRAGMENT = 1 << 0x02,
				CUSTODY_REQUESTED = 1 << 0x03,
				DESTINATION_IS_SINGLETON = 1 << 0x04,
				ACKOFAPP_REQUESTED = 1 << 0x05,
				RESERVED_6 = 1 << 0x06,
				PRIORITY_BIT1 = 1 << 0x07,
				PRIORITY_BIT2 = 1 << 0x08,
				CLASSOFSERVICE_9 = 1 << 0x09,
				CLASSOFSERVICE_10 = 1 << 0x0A,
				CLASSOFSERVICE_11 = 1 << 0x0B,
				CLASSOFSERVICE_12 = 1 << 0x0C,
				CLASSOFSERVICE_13 = 1 << 0x0D,
				REQUEST_REPORT_OF_BUNDLE_RECEPTION = 1 << 0x0E,
				REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE = 1 << 0x0F,
				REQUEST_REPORT_OF_BUNDLE_FORWARDING = 1 << 0x10,
				REQUEST_REPORT_OF_BUNDLE_DELIVERY = 1 << 0x11,
				REQUEST_REPORT_OF_BUNDLE_DELETION = 1 << 0x12,
				STATUS_REPORT_REQUEST_19 = 1 << 0x13,
				STATUS_REPORT_REQUEST_20 = 1 << 0x14,

				// DTNSEC FLAGS (these are customized flags and not written down in any draft)
				DTNSEC_REQUEST_SIGN = 1 << 0x1A,
				DTNSEC_REQUEST_ENCRYPT = 1 << 0x1B,
				DTNSEC_STATUS_VERIFIED = 1 << 0x1C,
				DTNSEC_STATUS_CONFIDENTIAL = 1 << 0x1D,
				DTNSEC_STATUS_AUTHENTICATED = 1 << 0x1E,
				IBRDTN_REQUEST_COMPRESSION = 1 << 0x1F
			};

			PrimaryBlock();
			virtual ~PrimaryBlock();

			/**
			 * This method is deprecated because it does not recognize the AgeBlock
			 * as alternative age verification.
			 */
			bool isExpired() const __attribute__ ((deprecated));

			std::string toString() const;

			void set(FLAGS flag, bool value);
			bool get(FLAGS flag) const;

			/**
			 * relabel the primary block with a new sequence number and a timestamp
			 */
			void relabel();

			bool operator==(const PrimaryBlock& other) const;
			bool operator!=(const PrimaryBlock& other) const;
			bool operator<(const PrimaryBlock& other) const;
			bool operator>(const PrimaryBlock& other) const;

			size_t _procflags;
			size_t _timestamp;
			size_t _sequencenumber;
			size_t _lifetime;
			size_t _fragmentoffset;
			size_t _appdatalength;

			EID _source;
			EID _destination;
			EID _reportto;
			EID _custodian;

		private:
			static ibrcommon::Mutex __sequence_lock;
			static size_t __sequencenumber;
		};
	}
}

#endif /* PRIMARYBLOCK_H_ */
