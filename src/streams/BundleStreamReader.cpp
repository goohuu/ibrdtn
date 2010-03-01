/*
 * BundleStreamReader.cpp
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/BundleStreamReader.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/Exceptions.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/Bundle.h"
#include <stdlib.h>

using namespace std;

namespace dtn
{
	namespace streams
	{
		BundleStreamReader::BundleStreamReader(istream &input)
		 : _input(input), _handler(_fake_handler)
		{
		}

		BundleStreamReader::BundleStreamReader(istream &input, BundleHandler &handler)
		 : _input(input), _handler(handler)
		{
		}

		BundleStreamReader::~BundleStreamReader()
		{
		}

		void BundleStreamReader::read(char *data, size_t size)
		{
			_input.read(data, size);
			if (_input.fail()) throw dtn::exceptions::IOException("read error");
		}

		size_t BundleStreamReader::readsome(char *data, size_t size)
		{
			size_t ret = _input.readsome(data, size);
			if (_input.fail()) throw dtn::exceptions::IOException("read error");
			return ret;
		}

		size_t BundleStreamReader::readChar(char &value)
		{
			_input.read(&value, sizeof(char));
			if (_input.fail()) throw dtn::exceptions::IOException("read error");
			return sizeof(char);
		}

		size_t BundleStreamReader::readSDNV(size_t &value)
		{
			dtn::data::SDNV sdnv;
			_input >> sdnv;
			if (_input.fail()) throw dtn::exceptions::IOException("read error");
			value = sdnv.getValue();
			return sdnv.getLength();
		}

		size_t BundleStreamReader::readBundle()
		{
			size_t pos = 0;
			char *data = NULL;
			size_t value = 0;

			char version;
			size_t proc_flags = 0;
			size_t header_length;
			size_t dict_length = 0;

			// VERSION
			pos += readChar(version);
			_handler.beginBundle(version);

			// check for input
			if (pos == 0) throw exceptions::InvalidBundleData("No data received.");

			// check for the right version
			if (version != dtn::data::BUNDLE_VERSION) throw exceptions::InvalidBundleData("Bundle version differ from ours.");

			// PROCFLAGS
			_handler.beginAttribute(BundleHandler::PROCFLAGS);
			pos += readSDNV(proc_flags);
			_handler.endAttribute(proc_flags);

			// BLOCK LENGTH
			_handler.beginAttribute(BundleHandler::BLOCKLENGTH);
			pos += readSDNV(header_length);
			_handler.endAttribute(header_length);

			// EIDs - LIFETIME
			for (int i = BundleHandler::DESTINATION_SCHEME; i <= BundleHandler::LIFETIME; i++)
			{
				_handler.beginAttribute(BundleHandler::ATTRIBUTES(i));
				pos += readSDNV(value);
				_handler.endAttribute(value);
			}

			// DICTIONARY_LENGTH
			_handler.beginAttribute(BundleHandler::DICTIONARY_LENGTH);
			pos += readSDNV(dict_length);
			_handler.endAttribute(dict_length);

			// DICTIONARY_BYTEARRAY
			_handler.beginAttribute(BundleHandler::DICTIONARY_BYTEARRAY);

			data = (char*)calloc(dict_length, sizeof(char));
			_input.read(data, dict_length); pos += dict_length;
			_handler.endAttribute(data, dict_length);

			free(data);

			// is this bundle a fragment?
			if (proc_flags & dtn::data::Bundle::FRAGMENT)
			{
				// FRAGMENTATION_OFFSET
				_handler.beginAttribute(BundleHandler::FRAGMENTATION_OFFSET);
				pos += readSDNV(value);
				_handler.endAttribute(value);

				// APPLICATION_DATA_LENGTH
				_handler.beginAttribute(BundleHandler::APPLICATION_DATA_LENGTH);
				pos += readSDNV(value);
				_handler.endAttribute(value);
			}

			bool lastblock = false;

			try {
				// read all BLOCKs
				while (!_input.eof() && !lastblock)
				{
					char block_type;
					size_t block_flags = 0;
					size_t block_length = 0;

					// BLOCK_TYPE
					pos += readChar(block_type);
					_handler.beginBlock(block_type);

					// BLOCK_FLAGS
					_handler.beginAttribute(BundleHandler::BLOCK_FLAGS);
					pos += readSDNV(block_flags);
					_handler.endAttribute(block_flags);

					// decode last block flag
					if (block_flags & dtn::data::Block::LAST_BLOCK) lastblock = true;

					// contains a reference block?
					if (block_flags & dtn::data::Block::BLOCK_CONTAINS_EIDS)
					{
						// BLOCK_REFERENCE_COUNT
						size_t ref_count = 0;
						_handler.beginAttribute(BundleHandler::BLOCK_REFERENCE_COUNT);
						pos += readSDNV(ref_count);
						_handler.endAttribute(ref_count);

						ref_count *= 2;
						while (ref_count > 0)
						{
							// BLOCK_REFERENCES
							_handler.beginAttribute(BundleHandler::BLOCK_REFERENCES);
							pos += readSDNV(value);
							_handler.endAttribute(value);

							ref_count--;
						}
					}

					// BLOCK_LENGTH
					_handler.beginAttribute(BundleHandler::BLOCK_LENGTH);
					pos += readSDNV(block_length);
					_handler.endAttribute(block_length);

					// BLOCK_PAYLOAD
					_handler.beginBlob();

					char data[512];
					size_t ret = 1;
					size_t step = 512;
					size_t remain = block_length;

					while (remain > 0)
					{
						if (remain > 512)
						{
							_input.read(data, 512);
							_handler.dataBlob(data, 512);
							pos += 512;
							remain -= 512;
						}
						else
						{
							_input.read(data, remain);
							_handler.dataBlob(data, remain);
							pos += remain;
							remain -= remain;
						}
					}

					_handler.endBlob(block_length);
					_handler.endBlock();
				}
			} catch (exceptions::InvalidDataException ex) {
				// end of bundle
			} catch (exceptions::IOException ex) {
				// read aborted. let the endBundle-Method check if there is enough
				// data for a fragment.
			}

			_handler.endBundle();

			return pos;
		}
	}
}
