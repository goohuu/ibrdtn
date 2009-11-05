/*
 * BundleStreamWriter.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef BUNDLESTREAMWRITER_H_
#define BUNDLESTREAMWRITER_H_

#include "ibrdtn/streams/BundleWriter.h"
#include "ibrdtn/data/SDNV.h"

namespace dtn
{
	namespace streams
	{
		class BundleStreamWriter : public BundleWriter
		{
		public:
			BundleStreamWriter(ostream &output);
			virtual ~BundleStreamWriter();

			size_t getSizeOf(u_int64_t value);
			size_t getSizeOf(string value);
			size_t getSizeOf(pair<size_t, size_t> value);

			size_t write(unsigned int value);
			size_t write(u_int64_t value);
			size_t write(int value);
			size_t write(pair<size_t, size_t> value);
			size_t write(double value);
			size_t write(char data);
			//size_t write(size_t value);
			size_t write(unsigned char data);
			size_t write(unsigned char *data, size_t length);
			size_t write(char *data, size_t length);
			size_t write(string value);
			size_t write(istream &input);
			size_t write(dtn::data::SDNV value);

			size_t write(dtn::blob::BLOBReference &ref);
			size_t write(const dtn::data::Bundle &b);

		private:
			size_t writeSDNV(u_int64_t value);
			ostream &_output;

		};
	}
}

#endif /* BUNDLESTREAMWRITER_H_ */
