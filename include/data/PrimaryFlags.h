#ifndef PRIMARYFLAGS_H_
#define PRIMARYFLAGS_H_

#include "ProcessingFlags.h"

namespace dtn
{
	namespace data
	{
		/**
		 * Specify the priority of a bundle.
		 */
		enum PriorityFlag
		{
			BULK = 0,
			NORMAL = 1,
			EXPEDITED = 2,
			RESERVED = 3
		};

		enum PrimaryProcBits
		{
			FRAGMENT = 0,
			APPDATA_IS_ADMRECORD = 1,
			DONT_FRAGMENT = 2,
			CUSTODY_REQUESTED = 3,
			DESTINATION_IS_SINGLETON = 4,
			ACKOFAPP_REQUESTED = 5,
			RESERVED_6 = 6,
			PRIORITY_BIT1 = 7,
			PRIORITY_BIT2 = 8,
			CLASSOFSERVICE_9 = 9,
			CLASSOFSERVICE_10 = 10,
			CLASSOFSERVICE_11 = 11,
			CLASSOFSERVICE_12 = 12,
			CLASSOFSERVICE_13 = 13,
			REQUEST_REPORT_OF_BUNDLE_RECEPTION = 14,
			REQUEST_REPORT_OF_CUSTODY_ACCEPTANCE = 15,
			REQUEST_REPORT_OF_BUNDLE_FORWARDING = 16,
			REQUEST_REPORT_OF_BUNDLE_DELIVERY = 17,
			REQUEST_REPORT_OF_BUNDLE_DELETION = 18,
			STATUS_REPORT_REQUEST_19 = 19,
			STATUS_REPORT_REQUEST_20 = 20
		};


		/**
		 * class to access the fields of the processing flags of the primary block.
		 */
		class PrimaryFlags : public data::ProcessingFlags
		{
		public:
			/**
			 * default constructor
			 */
			PrimaryFlags();

			/**
			 * constructor with a default value.
			 * @param value The value of the processing flags.
			 */
			PrimaryFlags(unsigned int value);

			/**
			 * destructor
			 */
			virtual ~PrimaryFlags();

			/**
			 * Determine if the bundle containts a fragment or not.
			 * @return true, if the bundle contains a fragment.
			 */
			bool isFragment();

			/**
			 * Set the fragment block bit.
			 * @param value set to true, if the bundle contains a fragment.
			 */
			void setFragment(bool value);

			/**
			 * Determine if the bundle contains a administrative record.
			 * @return true, if the bundle contains a administrative record.
			 */
			bool isAdmRecord();

			/**
			 * Set the administrative record bit.
			 * @param value set to true, if the bundle contains a administrative record.
			 */
			void setAdmRecord(bool value);

			/**
			 * Determine if fragmentation for this bundle is forbidden or not.
			 * @return true, if fragmentation if forbidden.
			 */
			bool isFragmentationForbidden();

			/**
			 * Set the fragmentation forbidden bit.
			 * @param value Set to true, if fragmentation should be forbidden for this bundle.
			 */
			void setFragmentationForbidden(bool value);

			/**
			 * Determine, if custody transfer is requested for this bundle.
			 * @return true, if custody transfer is requested.
			 */
			bool isCustodyRequested();

			/**
			 * Set the custody transfer requested bit.
			 * @param value Set to true, if custody transfer should be requested.
			 */
			void setCustodyRequested(bool value);

			/**
			 * Determine, if the destination EID is a singleton.
			 * @return true, if the destination EID is a singleton.
			 */
			bool isEIDSingleton();

			/**
			 * Set the destination EID is a singleton bit.
			 * @param value Set to true, if the destination EID is a singleton.
			 */
			void setEIDSingleton(bool value);

			/**
			 * Determine, if the acknowledgment of the application is requested.
			 * @return true, if the acknowledgment of the application is requested.
			 */
			bool isAckOfAppRequested();

			/**
			 * Set the application acknowledgment of application bit.
			 * @param value Set to true, if acknowledgment of the application should be requested.
			 */
			void setAckOfAppRequested(bool value);

			/**
			 * Get the priority of the bundle.
			 * @return A value for the priority of the bundle.
			 */
			PriorityFlag getPriority();

			/**
			 * Set the priority of the bundle.
			 * @param value The priority for the bundle.
			 */
			void setPriority(PriorityFlag value);
		};
	}
}

#endif /*PRIMARYFLAGS_H_*/
