#ifndef DISCOVERBLOCK_H_
#define DISCOVERBLOCK_H_

#include "data/Block.h"
#include "data/BlockFlags.h"
#include "data/Exceptions.h"

using namespace dtn::data;

namespace emma
{
	/**
	 * Ein DiscoverBlock kann an den Primärblock angehängt werden und wird
	 * verwendet um sich selbst gegenüber anderen DTN Knoten bemerkbar zu machen.
	 */
	class DiscoverBlock : public Block
	{
	public:
		static const unsigned char BLOCK_TYPE = 200;

		/**
		 * Konstruktor
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
