/*
 * BundleStreamBuf.h
 *
 *  Created on: 17.11.2011
 *      Author: morgenro
 */

#ifndef BUNDLESTREAMBUF_H_
#define BUNDLESTREAMBUF_H_

#include <ibrdtn/data/Bundle.h>
#include <ibrcommon/thread/Conditional.h>
#include <iostream>

namespace dtn
{
	namespace api
	{
		class BundleStreamBufCallback
		{
		public:
			virtual ~BundleStreamBufCallback() { };
			virtual void put(dtn::data::Bundle &b) = 0;
			virtual dtn::data::Bundle get(size_t timeout) = 0;
		};

		class BundleStreamBuf : public std::basic_streambuf<char, std::char_traits<char> >
		{
		public:
			// The size of the input and output buffers.
			static const size_t BUFF_SIZE = 5120;

			BundleStreamBuf(BundleStreamBufCallback &callback, size_t buffer = 4096, bool wait_seq_zero = false);
			virtual ~BundleStreamBuf();

//			virtual void received(const dtn::data::Bundle &b);

		protected:
			virtual int sync();
			virtual int overflow(int = std::char_traits<char>::eof());
			virtual int underflow();
//			int __underflow();

		private:
			class Chunk
			{
			public:
				Chunk(const dtn::data::Bundle &b);
				virtual ~Chunk();

				bool operator==(const Chunk& other) const;
				bool operator<(const Chunk& other) const;

				dtn::data::Bundle _bundle;
				size_t _seq;
			};

			void flushPayload();

			static void append(ibrcommon::BLOB::Reference &ref, const char* data, size_t length);
			static size_t getSequenceNumber(const dtn::data::Bundle &b);

			BundleStreamBufCallback &_callback;

			// Input buffer
			char *_in_buf;
			// Output buffer
			char *_out_buf;

			size_t _buffer;

			ibrcommon::BLOB::Reference _chunk_payload;

			std::set<Chunk> _chunks;
			size_t _chunk_offset;

			size_t _in_seq;
			size_t _out_seq;

			bool _streaming;

			size_t _timeout_receive;
		};
	} /* namespace data */
} /* namespace dtn */
#endif /* BUNDLESTREAMBUF_H_ */
