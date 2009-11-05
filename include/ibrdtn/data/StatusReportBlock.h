/*
 * StatusReportBlock.h
 *
 *  Created on: 05.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef STATUSREPORTBLOCK_H_
#define STATUSREPORTBLOCK_H_

#include "PayloadBlock.h"
#include "EID.h"

namespace dtn {
namespace data
{
	class StatusReportBlock : public PayloadBlock
	{
	public:
		enum TYPE
		{
			RECEIPT_OF_BUNDLE = 1 << 0,
			CUSTODY_ACCEPTANCE_OF_BUNDLE = 1 << 1,
			FORWARDING_OF_BUNDLE = 1 << 2,
			DELIVERY_OF_BUNDLE = 1 << 3,
			DELETION_OF_BUNDLE = 1 << 4
		};

		enum REASON_CODE
		{
			NO_ADDITIONAL_INFORMATION = 1 << 0x00,
			LIFETIME_EXPIRED = 1 << 0x01,
			FORWARDED_OVER_UNIDIRECTIONAL_LINK = 1 << 0x02,
			TRANSMISSION_CANCELED = 1 << 0x03,
			DEPLETED_STORAGE = 1 << 0x04,
			DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE = 1 << 0x05,
			NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE = 1 << 0x06,
			NO_TIMELY_CONTACT_WITH_NEXT_NODE_ON_ROUTE = 1 << 0x07,
			BLOCK_UNINTELLIGIBLE = 1 << 0x08
		};

		StatusReportBlock();
		StatusReportBlock(Block *block);
		StatusReportBlock(BLOBReference ref);
		virtual ~StatusReportBlock();

		void read();
		void commit();

		char _admfield;
		char _status;
		char _reasoncode;
		SDNV _fragment_offset;
		SDNV _fragment_length;
		SDNV _timeof_receipt;
		SDNV _timeof_custodyaccept;
		SDNV _timeof_forwarding;
		SDNV _timeof_delivery;
		SDNV _timeof_deletion;
		SDNV _bundle_timestamp;
		SDNV _bundle_sequence;
		EID _source;
	};
}
}

#endif /* STATUSREPORTBLOCK_H_ */
