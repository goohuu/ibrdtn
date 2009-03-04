#ifndef BLOCK_H_
#define BLOCK_H_

#include "BlockFlags.h"
#include "NetworkFrame.h"

namespace dtn
{
	namespace data
	{
		/**
		 * base class for blocks of a bundle.
		 */
		class Block
		{
		public:
			static const unsigned char BLOCK_TYPE = 0;

			/**
			 * Use a existing Block to create a new Block-Object.
			 * The used Block will be deleted.
			 * @param[in] block The block to use as base.
			 */
			Block(Block *block);

			/**
			 * Use a existing NetworkFrame to create a new Block-Object.
			 * @param[in] frame The NetworkFrame to use as base.
			 */
			Block(NetworkFrame *frame);

			/**
			 * Destructor
			 */
			virtual ~Block();

			/**
			 * get the reference to the NetworkFrame of this block
			 * @return the reference to the NetworkFrame of this block
			 */
			NetworkFrame& getFrame() const;

			/**
			 * Get the type of the block.
			 * @return The type of the block as char value.
			 */
			unsigned char getType() const;

			/**
			 * @return true, if the block can't be processed
			 */
			bool isProcessed() const;

			/**
			 * Get the processing flags of the block.
			 * @return A BlockFlags object containing the processing flags.
			 */
			BlockFlags getBlockFlags() const;

			/**
			 * Set the processing flags of the block.
			 * @param[in] flags A BlockFlags object containing the processing flags to set.
			 */
			void setBlockFlags(BlockFlags flags);

			/**
			 * Get the size of the header. This is the block size - payload size.
			 * @return the size of the header in bytes
			 */
			unsigned int getHeaderSize() const;

			/**
			 * dissociate the NetworkFrame from this object
			 * @return the pointer to the NetworkFrame of this block
			 */
			NetworkFrame* dissociateNetworkFrame();

			/**
			 * Updates the length field. This function should called after a set()-function.
			 */
			void updateBlockSize();

		protected:
			unsigned int getBodyIndex() const;
			bool m_processed;

		private:
			NetworkFrame *m_frame;
		};
	}
}

#endif /*BLOCK_H_*/
