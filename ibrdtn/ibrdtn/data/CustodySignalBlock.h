/*
 * CustodySignalBlock.h
 *
 *  Created on: 05.06.2009
 *      Author: morgenro
 */



#ifndef CUSTODYSIGNALBLOCK_H_
#define CUSTODYSIGNALBLOCK_H_

#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrcommon/data/BLOB.h"
#include "ibrdtn/data/DTNTime.h"

namespace dtn
{
	namespace data
	{
		class CustodySignalBlock : public Block
		{
		public:
			CustodySignalBlock();
			virtual ~CustodySignalBlock();

			void setMatch(const Bundle& other);
			bool match(const Bundle& other) const;

			virtual size_t getLength() const;
			virtual std::ostream &serialize(std::ostream &stream) const;
			virtual std::istream &deserialize(std::istream &stream);

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
