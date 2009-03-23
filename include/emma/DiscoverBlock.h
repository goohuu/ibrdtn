#ifndef DISCOVERBLOCK_H_
#define DISCOVERBLOCK_H_

#include "data/Block.h"
#include "data/BlockFlags.h"
#include "data/Exceptions.h"

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

		/**
		 * constructor
		 */
		DiscoverBlock(NetworkFrame *frame);
		DiscoverBlock(Block *block);

		/**
		 * destructor
		 */
		virtual ~DiscoverBlock();

		string getConnectionAddress();
		void setConnectionAddress(string address);

		unsigned int getConnectionPort();
		void setConnectionPort(unsigned int port);

		unsigned int getOptionals();

		void setLatitude(double value);
		double getLatitude();
		void setLongitude(double value);
		double getLongitude();
	};
}

#endif /*DISCOVERBLOCK_H_*/
