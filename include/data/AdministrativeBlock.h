#ifndef ADMINISTRATIVEBLOCK_H_
#define ADMINISTRATIVEBLOCK_H_

#include "NetworkFrame.h"
#include "PayloadBlock.h"

namespace dtn
{
namespace data
{
	/**
	 * possible block types for administrative blocks
	 */
	enum AdministrativeBlockType
	{
		UNKNOWN = 0,
		STATUS_REPORT = 1,
		CUSTODY_SIGNAL = 2
	};

	/**
	 * base class for administrative blocks
	 */
	class AdministrativeBlock : public PayloadBlock
	{
		public:
			/**
			 * Use a existing Block to create a new AdministrativeBlock-Object.
			 * The used Block will be deleted.
			 * @param[in] block The block to use as base.
			 * @param[in] type The type of the AdministrativeBlock to create.
			 */
			AdministrativeBlock(Block *block, AdministrativeBlockType type);

			/**
			 * Use a existing NetworkFrame to create a new AdministrativeBlock-Object.
			 * @param[in] frame The NetworkFrame to use as base.
			 * @param[in] type The type of the AdministrativeBlock to create.
			 */
			AdministrativeBlock(NetworkFrame *frame, AdministrativeBlockType type);

			/**
			 * Destructor for the AdministrativeBlock.
			 */
			virtual ~AdministrativeBlock();

			/**
			 * Get the type of a AdministrativeBlock-Instance.
			 * @return A type of the AdministrationBlockType enumeration.
			 */
			AdministrativeBlockType getType() const;

			/**
			 * Get the StatusFlags as a ProcessingFlags-Object.
			 * @return The StatusFlags as object.
			 */
			ProcessingFlags getStatusFlags() const;

			/**
			 * Set the status flags of the block.
			 * @param[in] flags The status flags to set.
			 */
			void setStatusFlags(ProcessingFlags flags);

			/**
			 * Identify a PayloadBlock which is a AdministrativeBlock
			 * of a specific type.
			 * @return The type of the block represented by a
			 * AdministrationBlockType enumeration.
			 */
			static AdministrativeBlockType identify(PayloadBlock *block);

		private:
			AdministrativeBlockType m_type;
	};
}
}

#endif /*ADMINISTRATIVEBLOCK_H_*/
