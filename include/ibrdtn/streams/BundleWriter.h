/*
 * BundleWriter.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#ifndef BUNDLEWRITER_H_
#define BUNDLEWRITER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/BLOBReference.h"

namespace dtn
{
	namespace data
	{
		class Bundle;
		class Block;
	}

	namespace streams
	{
		class BundleWriter
		{
			friend class Bundle;
			friend class Block;

		public:
			virtual size_t getSizeOf(u_int64_t value) = 0;
			virtual size_t getSizeOf(string value) = 0;
			virtual size_t getSizeOf(pair<size_t, size_t> value) = 0;

			virtual size_t write(unsigned int value) = 0;	// write a SDNV
			virtual size_t write(u_int64_t value) = 0;		// write a SDNV
			virtual size_t write(int value) = 0;		// write a SDNV
			virtual size_t write(pair<size_t, size_t> value) = 0; // write two SDNVs
			virtual size_t write(double value) = 0;
			virtual size_t write(char data) = 0;
			//virtual size_t write(size_t value) = 0;
			virtual size_t write(unsigned char data) = 0;
			virtual size_t write(unsigned char *data, size_t length) = 0;
			virtual size_t write(char *data, size_t length) = 0;
			virtual size_t write(string value) = 0;
			virtual size_t write(istream &input) = 0;

			virtual size_t write(dtn::blob::BLOBReference &ref) = 0;
			virtual size_t write(const dtn::data::Bundle &b) = 0;
		};
	}
}

#endif /* BUNDLEWRITER_H_ */
