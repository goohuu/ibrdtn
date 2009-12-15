/*
 * CustodySignalBlock.h
 *
 *  Created on: 05.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/default.h"

#ifndef CUSTODYSIGNALBLOCK_H_
#define CUSTODYSIGNALBLOCK_H_

#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrcommon/data/BLOBReference.h"
#include "ibrdtn/data/DTNTime.h"

namespace dtn
{
	namespace data
	{
		class CustodySignalBlock : public PayloadBlock
		{
		public:
			CustodySignalBlock();
			CustodySignalBlock(Block *block);
			CustodySignalBlock(ibrcommon::BLOBReference ref);
			virtual ~CustodySignalBlock();

			void read();
			void commit();

			void setMatch(const Bundle& other);
			bool match(const Bundle& other) const;

			char _admfield;
			char _status;
			SDNV _fragment_offset;
			SDNV _fragment_length;
			DTNTime _timeofsignal;
			SDNV _bundle_timestamp;
			SDNV _bundle_sequence;
			EID _source;
		};
	}
}

#endif /* CUSTODYSIGNALBLOCK_H_ */
