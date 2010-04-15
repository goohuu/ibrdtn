/*
 * BundleStreamReader.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef BUNDLESTREAMREADER_H_
#define BUNDLESTREAMREADER_H_

#include "ibrdtn/streams/BundleHandler.h"
#include "ibrdtn/streams/FakeBundleHandler.h"
#include <iostream>

namespace dtn
{
	namespace streams
	{
		class BundleStreamReader
		{
		public:
			BundleStreamReader(istream &input);
			BundleStreamReader(istream &input, BundleHandler &handler);
			virtual ~BundleStreamReader();
			void read(char *data, size_t size);
			size_t readsome(char *data, size_t size);
			size_t readBundle();
			size_t readChar(char &value);
			size_t readSDNV(size_t &value);

		private:
			enum FIELDS
			{
				DO_NOT_PARSE = -1,
				ATTRIBUTE_CHAR = 0,
				ATTRIBUTE_SDNV = 1,
				BYTEARRAY = 2,
				BLOB = 3
			};

			FakeBundleHandler _fake_handler;

			istream &_input;
			BundleHandler &_handler;
		};
	}
}

#endif /* BUNDLESTREAMREADER_H_ */
