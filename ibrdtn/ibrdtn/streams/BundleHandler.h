/*
 * BundleHandler.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#ifndef BUNDLEHANDLER_H_
#define BUNDLEHANDLER_H_

#include "ibrdtn/default.h"
#include <stddef.h>

using namespace std;

namespace dtn
{
	namespace streams
	{
		class BundleHandler
		{
		public:
			enum ATTRIBUTES
			{
				VERSION = 1,
				PROCFLAGS = 2,
				BLOCKLENGTH = 3,
				DESTINATION_SCHEME = 4,
				DESTINATION_SSP = 5,
				SOURCE_SCHEME = 6,
				SOURCE_SSP = 7,
				REPORTTO_SCHEME = 8,
				REPORTTO_SSP = 9,
				CUSTODIAN_SCHEME = 10,
				CUSTODIAN_SSP = 11,
				CREATION_TIMESTAMP = 12,
				CREATION_TIMESTAMP_SEQUENCE = 13,
				LIFETIME = 14,
				DICTIONARY_LENGTH = 15,
				DICTIONARY_BYTEARRAY = 16,
				FRAGMENTATION_OFFSET = 17,
				APPLICATION_DATA_LENGTH = 18,
				BLOCK_TYPE = 19,
				BLOCK_FLAGS = 20,
				BLOCK_REFERENCE_COUNT = 21,
				BLOCK_REFERENCES = 22,
				BLOCK_LENGTH = 23,
				BLOCK_PAYLOAD = 24,
				BLOCK_END = 25
			};

			virtual void beginBundle(char version) = 0;

			/**
			 * This method checks the received data. If the bundle is not received
			 * completely then transform it into a fragment.
			 */
			virtual void endBundle() = 0;

			virtual void beginAttribute(ATTRIBUTES attr) = 0;
			virtual void endAttribute(char value) = 0;
			virtual void endAttribute(char *data, size_t size) = 0;
			virtual void endAttribute(size_t value) = 0;
			virtual void beginBlock(char type) = 0;
			virtual void endBlock() = 0;
			virtual void beginBlob() = 0;
			virtual void dataBlob(char *data, size_t length) = 0;
			virtual void endBlob(size_t size) = 0;
		};
	}
}

#endif /* BUNDLEHANDLER_H_ */
