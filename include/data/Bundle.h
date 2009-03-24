#ifndef BUNDLE_H_
#define BUNDLE_H_

#include <sys/types.h>
#include "Block.h"
#include "Exceptions.h"
#include "Dictionary.h"
#include "NetworkFrame.h"
#include "PrimaryFlags.h"
#include "AdministrativeBlock.h"
#include <list>

using namespace dtn::exceptions;
using namespace std;

namespace dtn
{
	namespace data
	{
		/**
		 * enumeration of all fields in the primary block of a bundle.
		 */
		enum BUNDLE_FIELDS
		{
			VERSION = 0,
			PROCFLAGS = 1,
			BLOCKLENGTH = 2,
			DESTINATION_SCHEME = 3,
			DESTINATION_SSP = 4,
			SOURCE_SCHEME = 5,
			SOURCE_SSP = 6,
			REPORTTO_SCHEME = 7,
			REPORTTO_SSP = 8,
			CUSTODIAN_SCHEME = 9,
			CUSTODIAN_SSP = 10,
			CREATION_TIMESTAMP = 11,
			CREATION_TIMESTAMP_SEQUENCE = 12,
			LIFETIME = 13,
			DICTIONARY_LENGTH = 14,
			DICTIONARY_BYTEARRAY = 15,
			FRAGMENTATION_OFFSET = 16,
			APPLICATION_DATA_LENGTH = 17
		};

		/**
		 * version of this bundle protocol implementation
		 */
		static const unsigned char BUNDLE_VERSION = 0x06;

		/**
		 * create a bundle object to manage the parameter and blocks of a bundle.
		 */
		class Bundle
		{
		public:
			/**
			 * creates a new bundle object with a given NetworkFrame
			 * @param[in] frame A NetworkFrame Object with parsed data.
			 */
			Bundle(NetworkFrame *frame);

			/**
			 * creates a new bundle object with a given NetworkFrame and Blocks
			 * @param[in] frame A NetworkFrame Object with parsed data.
			 * @param[in] blocks A list of block references which belongs to this bundle.
			 */
			Bundle(NetworkFrame *frame, const list<Block*> blocks);

			Bundle(const Bundle &b);

			/**
			 * destructor, called if a bundle is destroyed
			 */
			virtual ~Bundle();

			/**
			 * Compare a Bundle object to another one.
			 * A bundle is equal to another if the source, creation timestamp and
			 * the sequence number is equal.
			 * @param[in] other The bundle to compare with this bundle.
			 * @returns true, if the bundle is equal to the given one.
			 */
			bool operator==(const Bundle& other) const;

			/**
			 * Compare a Bundle object to another one.
			 * @sa operator==()
			 * @param[in] other The bundle to compare with this bundle.
			 * @returns true, if the bundle is NOT equal to the given one.
			 */
			bool operator!=(const Bundle& other) const;

			/**
			 * Mark a bundle as containing fragmented payload blocks.
			 * The required fields FRAGMENTATION_OFFSET and APPLICATION_DATA_LENGTH
			 * will be added or removed if necessary.
			 * @param[in] value true, if this bundle should marked as containing
			 * fragmented payload blocks.
			 */
			void setFragment(bool value);

			/**
			 * Get a data-array with a copy of the hole bundle with all trailing blocks.
			 * You need to free this array after use!
			 * @return A data-array with all bundle data.
			 * @sa getLength()
			 */
			unsigned char* getData() const;

			/**
			 * Get the length of the hole bundle with all trailing blocks
			 * @return The length of the hole bundle
			 * @sa getData()
			 */
			unsigned int getLength() const;

			/**
			 * Get a numeric value of a specific field.
			 *
			 * Exceptions:
			 * InvalidFieldException 	The field doesn't contain a numeric value.
			 * FieldDoesNotExists		The field doesn't exists.
			 * InvalidBundleData		The data of the bundle is invalid.
			 * SDNVDecodeFailed			Decoding of the SDNV failed.
			 *
			 * @param[in] field A field out of the enumeration BUNDLE_FIELDS
			 * @return The numeric value of the field.
			 */
			u_int64_t getInteger(const BUNDLE_FIELDS field) const throw (InvalidFieldException, FieldDoesNotExist);

			/**
			 * Set a numeric value of a specific field.
			 * @param[in] field A field out of the enumeration BUNDLE_FIELD
			 * @param[in] value The numeric value to set in the field.
			 */
			void setInteger(const BUNDLE_FIELDS field, const u_int64_t value) throw (InvalidFieldException, FieldDoesNotExist);

			/**
			 * Get the destination EID of the bundle.
			 * @return The destination EID of the bundle.
			 */
			string getDestination() const;

			/**
			 * Set the destination EID of the bundle.
			 * @param[in] destination The destination EID to set.
			 */
			void setDestination(string destination);

			/**
			 * Get the source EID of the bundle.
			 * @return The source EID of the bundle.
			 */
			string getSource() const;

			/**
			 * Set the source EID of the bundle.
			 * @param[in] source The source EID to set.
			 */
			void setSource(string source);

			/**
			 * Get the reportto EID of the bundle.
			 * @return The reportto EID of the bundle.
			 */
			string getReportTo() const;

			/**
			 * Set the reportto EID of the bundle.
			 * @param[in] reportto The reportto EID to set.
			 */
			void setReportTo(string reportto);

			/**
			 * Get the custodian EID of the bundle.
			 * @return The custodian EID of the bundle.
			 */
			string getCustodian() const;

			/**
			 * Set the custodian EID of the bundle.
			 * @param[in] custodian The custodian EID to set.
			 */
			void setCustodian(string custodian);

			/**
			 * Get the primary flags of this bundle encapsulated
			 * in a PrimaryFlags object.
			 * @return A PrimaryFlags object which contains the flags
			 * of the bundle.
			 */
			PrimaryFlags getPrimaryFlags() const;

			/**
			 * Set the primary flags of this bundle.
			 * @param[in] flags PrimaryFlags objects containing the flags to set.
			 */
			void setPrimaryFlags(PrimaryFlags &flags);

			/**
			 * Get a list of all blocks of this bundle.
			 * @return a list of blocks of this bundle.
			 */
			const list<Block*> &getBlocks() const;

			/**
			 * Append a new block at the end of this bundle.
			 * @param[in] block The block to append.
			 */
			void appendBlock(Block *block);

			/**
			 * Insert a new block in front of the first block of this bundle.
			 * @param[in] block The block to insert.
			 */
			void insertBlock(Block *block);

			/**
			 * Remove a specific block of the bundle.
			 * @param[in] block The pointer to the bundle to remove.
			 */
			void removeBlock(Block *block);

			/**
			 * Get the NetworkFrame of this bundle containing the primary block.
			 * @return The NetworkFrame of this bundle.
			 */
			const NetworkFrame &getFrame() const;

			/**
			 * Updates the BLOCKLENGTH field. This function should called after a set()-function.
			 */
			void updateBlockLength();

			/**
			 * Determine if a bundle is expired.
			 * return true, if the bundle is expired, else false.
			 */
			bool isExpired() const;

			/**
			 * Create a short string to represent this bundle.
			 * @returns a short string to represent this bundle.
			 */
			string toString() const;

			/**
			 * Search and return the PayloadBlock of a bundle.
			 * @return the PayloadBlock of a bundle and NULL if no PayloadBlock is present.
			 */
			PayloadBlock* getPayloadBlock() const;

			/**
			 * Get a list of block with a specific block type
			 * @return a list of block objects
			 */
			list<Block*> getBlocks(const unsigned char type) const;

#ifdef DO_DEBUG_OUTPUT
			void debug() const;
#endif

		private:
			/**
			 * Get the position of a field in the raw data.
			 * The position is the number of bytes from begin of the raw data.
			 *
			 * Exceptions:
			 * FieldDoesNotExists		Field doesn't exists
			 * InvalidBundleData		The bundle data is invalid.
			 *
			 * @param[in] field A field out of the enumeration BUNDLE_FIELDS
			 * @return The numeric position of the field.
			 */
			int getPosition(BUNDLE_FIELDS field) throw (FieldDoesNotExist, InvalidBundleData);

			NetworkFrame *m_frame;

			Dictionary getDictionary() const;
			void commitDictionary(const Dictionary &dict);

			list<Block*> m_blocks;
		};
	}
}

#endif /*BUNDLE_H_*/
