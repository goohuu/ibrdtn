/*
 * SecurityBlock.h
 *
 *  Created on: 08.03.2010
 *      Author: morgenro
 */

#ifndef SECURITYBLOCK_H_
#define SECURITYBLOCK_H_

#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/EID.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/BundleString.h"
#include <ibrcommon/data/BLOB.h>
#include <list>

namespace dtn
{
	namespace data
	{
		class SecurityBlock : public Block
		{
		public:
			enum BLOCK_TYPES
			{
				BUNDLE_AUTHENTICATION_BLOCK = 0x02,
				PAYLOAD_INTEGRITY_BLOCK = 0x03,
				PAYLOAD_CONFIDENTIAL_BLOCK = 0x04,
				EXTENSION_SECURITY_BLOCK = 0x09
			};

			enum CIPHERSUITE_FLAGS
			{
				CONTAINS_SECURITY_RESULT = 1 << 0,
				CONTAINS_CORRELATOR = 1 << 1,
				CONTAINS_CIPHERSUITE_PARAMS = 1 << 2,
				CONTINAS_SECURITY_DESTINATION = 1 << 3,
				CONTINAS_SECURITY_SOURCE = 1 << 4,
				BIT5_RESERVED = 1 << 5,
				BIT6_RESERVED = 1 << 6
			};

			virtual ~SecurityBlock() = 0;

			virtual std::list<dtn::data::EID> getEIDList() const;

		protected:
			SecurityBlock(SecurityBlock::BLOCK_TYPES type);
			SecurityBlock(SecurityBlock::BLOCK_TYPES type, ibrcommon::BLOB::Reference ref);

			virtual void read();
			virtual void commit();

			dtn::data::EID _security_source;
			dtn::data::EID _security_destination;

			dtn::data::SDNV _ciphersuite_id;
			dtn::data::SDNV _ciphersuite_flags;
			dtn::data::SDNV _correlator; // optional

			dtn::data::BundleString _ciphersuite_params; // optional
			dtn::data::BundleString _security_result; // optional

		private:
			/**
			 * Add EID is not allowed on SecurityBlocks
			 */
			virtual void addEID(dtn::data::EID eid) {};
		};

		class BundleAuthenticationBlock : public SecurityBlock
		{
		public:
			static const char BLOCK_TYPE = SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK;
		};

		class PayloadIntegrityBlock : public SecurityBlock
		{
		public:
			static const char BLOCK_TYPE = SecurityBlock::PAYLOAD_INTEGRITY_BLOCK;
		};

		class PayloadConfidentialBlock : public SecurityBlock
		{
		public:
			static const char BLOCK_TYPE = SecurityBlock::PAYLOAD_CONFIDENTIAL_BLOCK;
		};
	}
}


#endif /* SECURITYBLOCK_H_ */
