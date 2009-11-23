/*
 * Block.h
 *
 *  Created on: 29.05.2009
 *      Author: morgenro
 */

#ifndef BLOCK_H_
#define BLOCK_H_

#include "ibrdtn/default.h"
#include "ibrdtn/streams/BundleWriter.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/data/BLOBReference.h"
#include "ibrdtn/data/EID.h"

namespace dtn
{
	namespace data
	{
		class Bundle;

		class Block
		{
			friend class Bundle;
			friend class dtn::streams::BundleStreamWriter;

		public:
			Block(char blocktype);
			Block(char blocktype, blob::BLOBManager::BLOB_TYPE type);
			Block(char blocktype, blob::BLOBReference ref);
			virtual ~Block();

			virtual void addEID(EID eid);
			virtual list<EID> getEIDList() const;

			size_t getSize() const;
			blob::BLOBReference getBLOBReference();

			size_t _procflags;

			virtual void read() {}
			virtual void commit() {};

			char getType() { return _blocktype; }

		protected:
			size_t writeHeader( dtn::streams::BundleWriter &writer ) const;

			char _blocktype;
			list<EID> _eids;
			blob::BLOBReference _blobref;

			virtual size_t write( dtn::streams::BundleWriter &writer );
		};
	}
}

#endif /* BLOCK_H_ */
