#ifndef STATUSREPORTBLOCK_H_
#define STATUSREPORTBLOCK_H_

#include "data/AdministrativeBlock.h"
#include "data/ProcessingFlags.h"
#include "data/Exceptions.h"

namespace dtn
{
	namespace data
	{
		enum STATUSREPORT_FIELDS
		{
			STATUSREPORT_ADMFIELD = 0,
			STATUSREPORT_STATUS = 1,
			STATUSREPORT_REASON = 2,
			STATUSREPORT_FRAGMENT_OFFSET = 3,
			STATUSREPORT_FRAGMENT_LENGTH = 4,
			STATUSREPORT_TIMEOF_RECEIPT = 5,
			STATUSREPORT_TIMEOF_CUSTODYACCEPT = 6,
			STATUSREPORT_TIMEOF_FORWARDING = 7,
			STATUSREPORT_TIMEOF_DELIVERY = 8,
			STATUSREPORT_TIMEOF_DELETION = 9,
			STATUSREPORT_BUNDLE_TIMESTAMP = 10,
			STATUSREPORT_BUNDLE_SEQUENCE = 11,
			STATUSREPORT_BUNDLE_SOURCE_LENGTH = 12,
			STATUSREPORT_BUNDLE_SOURCE = 13
		};

		enum StatusReportType
		{
			RECEIPT_OF_BUNDLE = 0,
			CUSTODY_ACCEPTANCE_OF_BUNDLE = 1,
			FORWARDING_OF_BUNDLE = 2,
			DELIVERY_OF_BUNDLE = 3,
			DELETION_OF_BUNDLE = 4
		};

		enum StatusReportReasonCode
		{
			NO_ADDITIONAL_INFORMATION = 0x00,
			LIFETIME_EXPIRED = 0x01,
			FORWARDED_OVER_UNIDIRECTIONAL_LINK = 0x02,
			TRANSMISSION_CANCELED = 0x03,
			DEPLETED_STORAGE = 0x04,
			DESTINATION_ENDPOINT_ID_UNINTELLIGIBLE = 0x05,
			NO_KNOWN_ROUTE_TO_DESTINATION_FROM_HERE = 0x06,
			NO_TIMELY_CONTACT_WITH_NEXT_NODE_ON_ROUTE = 0x07,
			BLOCK_UNINTELLIGIBLE = 0x08
		};

		class StatusReportBlock : public data::AdministrativeBlock
		{
		public:
			/**
			 * constructor
			 */
			StatusReportBlock(Block *block);
			StatusReportBlock(NetworkFrame *frame);

			/**
			 * destructor
			 */
			virtual ~StatusReportBlock();

			bool forFragment() const;

			unsigned int getFragmentOffset() const;
			void setFragmentOffset(unsigned int value);

			unsigned int getFragmentLength() const;
			void setFragmentLength(unsigned int value);

			// ReasonCode
			ProcessingFlags getReasonCode() const;
			void setReasonCode(ProcessingFlags value);

			// Time of receipt of bundle
			unsigned int getTimeOfReceipt() const;
			void setTimeOfReceipt(unsigned int value);

			// Time of custody acceptance of bundle
			unsigned int getTimeOfCustodyAcceptance() const;
			void setTimeOfCustodyAcceptance(unsigned int value);

			// Time of forwarding of bundle
			unsigned int getTimeOfForwarding() const;
			void setTimeOfForwarding(unsigned int value);

			// Time of delivery of bundle
			unsigned int getTimeOfDelivery() const;
			void setTimeOfDelivery(unsigned int value);

			// Time of deletion
			unsigned int getTimeOfDeletion() const;
			void setTimeOfDeletion(unsigned int value);

			// Copy of bundles creation timestamp
			unsigned int getCreationTimestamp() const;
			void setCreationTimestamp(unsigned int value);

			// Copy of bundles creation timestamp sequence number
			unsigned int getCreationTimestampSequence() const;
			void setCreationTimestampSequence(unsigned int value);

			bool match(const Bundle &b) const;
			void setMatch(const Bundle &b);

			// Source endpoint ID of Bundle
			string getSource() const;
			void setSource(string value);

		private:
			unsigned int getField(STATUSREPORT_FIELDS field) const;
		};
	}
}

#endif /*STATUSREPORTBLOCK_H_*/
