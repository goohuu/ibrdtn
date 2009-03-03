#ifndef BLOCKFLAGS_H_
#define BLOCKFLAGS_H_

#include "ProcessingFlags.h"

namespace dtn
{
	namespace data
	{
		/**
		 * Possible flags of a block.
		 */
		enum BlockProcBits
		{
			REPLICATE_IF_FRAGMENTED = 0,
			REPORT_IF_CANT_PROCESSED = 1,
			DELETE_IF_CANT_PROCESSED = 2,
			LAST_BLOCK = 3,
			DISCARD_IF_CANT_PROCESSED = 4,
			FORWARDED_WITHOUT_PROCESSED = 5,
			CONTAINS_EID_FIELD = 6
		};

		/**
		 * Encapsulate the block flags into a object and returns or read
		 * a integer value containing the flags.
		 */
		class BlockFlags : public data::ProcessingFlags
		{
		public:
			/**
			 * Constructor for the BlockFlags with all flags set to zero.
			 */
			BlockFlags();

			/**
			 * Constructor for the BlockFlags with a default value.
			 * @param value Value to read the flags from.
			 */
			BlockFlags(unsigned int value);

			/**
			 * Destructor
			 */
			virtual ~BlockFlags();
		};
	}
}

#endif /*BLOCKFLAGS_H_*/
