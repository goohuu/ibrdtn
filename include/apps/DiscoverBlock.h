#ifndef DISCOVERBLOCK_H_
#define DISCOVERBLOCK_H_

#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/Exceptions.h"

using namespace dtn::data;

namespace emma
{
	/**
	 * A DiscoverBlock carry some information about the sending node and can be
	 * attached to a bundle. The lifetime of a bundle carrying this block should be
	 * set to zero to avoid a forwarding to other nodes than direct neighbors.
	 */
	class DiscoverBlock : public Block
	{
	public:
		static const unsigned char BLOCK_TYPE = 200;

		DiscoverBlock();
		DiscoverBlock(dtn::blob::BLOBReference ref);
		DiscoverBlock(Block *block);
		virtual ~DiscoverBlock();

		string _address;
		unsigned int _port;

		void read();
		void commit();
	};
}

#endif /*DISCOVERBLOCK_H_*/
