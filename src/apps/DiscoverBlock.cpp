#include "emma/DiscoverBlock.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/data/BLOBReference.h"
#include "ibrdtn/data/SDNV.h"

#include <cstdlib>
#include <cstring>

using namespace dtn::data;
using namespace dtn::blob;

namespace emma
{
	DiscoverBlock::DiscoverBlock()
	 : Block(BLOCK_TYPE, BLOBManager::BLOB_MEMORY)
	{
	}

	DiscoverBlock::DiscoverBlock(Block *block)
	 : Block(BLOCK_TYPE, block->getBLOBReference())
	{
		read();
	}

	DiscoverBlock::DiscoverBlock(dtn::blob::BLOBReference ref)
	 : Block(BLOCK_TYPE, ref)
	{
	}

	DiscoverBlock::~DiscoverBlock()
	{
	}

	void DiscoverBlock::read()
	{
		// read the attributes out of the BLOBReference
		BLOBReference ref = Block::getBLOBReference();
		size_t remain = ref.getSize();
		char buffer[remain];

		// read the data into the buffer
		ref.read(buffer, 0, remain);

		// make a useable pointer
		char *data = buffer;

		u_int64_t value = 0;
		size_t len = 0;

		len = SDNV::decode(data, remain, &_port); remain -= len; data += len;
		len = SDNV::decode(data, remain, &value); remain -= len; data += len;

		string source;
		_address.assign(data, value);
	}

	void DiscoverBlock::commit()
	{
		stringstream ss;
		BundleStreamWriter w(ss);

		w.write(_port);				// own port (e.g. port)
		w.write(_address.length()); // the length of the own address
		w.write(_address);			// own address (e.g. ip address)
		w.write(0); 				// optionals: none

		// clear the blob
		BLOBReference ref = Block::getBLOBReference();
		ref.clear();

		// copy the content of ss to the blob
		string data = ss.str();
		ref.append(data.c_str(), data.length());
	}
}
