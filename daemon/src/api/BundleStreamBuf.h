/*
 * BundleStreamBuf.h
 *
 *  Created on: 17.11.2011
 *      Author: morgenro
 */

#ifndef BUNDLESTREAMBUF_H_
#define BUNDLESTREAMBUF_H_

#include <ibrdtn/data/MetaBundle.h>
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
			virtual dtn::data::MetaBundle get(size_t timeout = 0) = 0;
			virtual void delivered(const dtn::data::MetaBundle &b) = 0;
		};

		class BundleStreamBuf : public std::basic_streambuf<char, std::char_traits<char> >
		{
		public:
			// The size of the input and output buffers.
			static const size_t BUFF_SIZE = 5120;

			BundleStreamBuf(BundleStreamBufCallback &callback, size_t chunk_size = 4096, bool wait_seq_zero = false);
			virtual ~BundleStreamBuf();

			void setChunkSize(size_t size);
			void setTimeout(size_t timeout);

		protected:
			virtual int sync();
			virtual int overflow(int = std::char_traits<char>::eof());
			virtual int underflow();

		private:
			class Chunk
			{
			public:
				Chunk(const dtn::data::MetaBundle &m);
				virtual ~Chunk();

				bool operator==(const Chunk& other) const;
				bool operator<(const Chunk& other) const;

				void load();

				dtn::data::MetaBundle _meta;
				size_t _seq;
				bool _first;
				bool _last;
			};

			void flushPayload(bool final = false);

			static void append(ibrcommon::BLOB::Reference &ref, const char* data, size_t length);

			BundleStreamBufCallback &_callback;

			// Input buffer
			char *_in_buf;
			// Output buffer
			char *_out_buf;

			size_t _chunk_size;

			ibrcommon::BLOB::Reference _chunk_payload;

			std::set<Chunk> _chunks;
			size_t _chunk_offset;

			dtn::data::Bundle _current_bundle;

			size_t _in_seq;
			size_t _out_seq;

			bool _streaming;

			// is set to true, when the first bundle is still not sent
			bool _first_chunk;

			// if true, the last chunk was received before
			bool _last_chunk_received;

			size_t _timeout_receive;
		};
	} /* namespace data */
} /* namespace dtn */
#endif /* BUNDLESTREAMBUF_H_ */
