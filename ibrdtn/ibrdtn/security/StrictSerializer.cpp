#include "ibrdtn/security/StrictSerializer.h"
#include "ibrdtn/security/BundleAuthenticationBlock.h"
#include "ibrdtn/security/PayloadIntegrityBlock.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/Bundle.h"

namespace dtn
{
	namespace security
	{
		StrictSerializer::StrictSerializer(std::ostream& stream)
		 : DefaultSerializer(stream)
		{
		}

		StrictSerializer::~StrictSerializer()
		{
		}

		StrictSerializer& StrictSerializer::serialize_strict(const dtn::data::Bundle& bundle, const dtn::security::SecurityBlock::BLOCK_TYPES type, const bool with_correlator, const u_int64_t correlator)
		{
			if (type == dtn::security::SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK)
			{
				std::list<const BundleAuthenticationBlock*> babs = bundle.getBlocks<BundleAuthenticationBlock>();
				for (std::list<const BundleAuthenticationBlock*>::const_iterator it = babs.begin(); it != babs.end(); it++)
					(*it)->set_ignore_security_result(true);
			}
			else if (type == dtn::security::SecurityBlock::PAYLOAD_INTEGRITY_BLOCK)
			{
				std::list<const PayloadIntegrityBlock*> babs = bundle.getBlocks<PayloadIntegrityBlock>();
				for (std::list<const PayloadIntegrityBlock*>::const_iterator it = babs.begin(); it != babs.end(); it++)
					(*it)->set_ignore_security_result(true);
			}

			// rebuild the dictionary
			rebuildDictionary(bundle);

			// serialize the primary block
			(*this) << static_cast<const dtn::data::PrimaryBlock&>(bundle);

			// serialize all secondary blocks
			std::list<refcnt_ptr<dtn::data::Block> > list = bundle._blocks._blocks;
			std::list<refcnt_ptr<dtn::data::Block> >::const_iterator iter = list.begin();

			// skip all blocks before the correlator
			for (; with_correlator && iter != list.end(); iter++)
			{
				const dtn::data::Block &b = (*(*iter));
				if (b.getType() == SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK || b.getType() == SecurityBlock::PAYLOAD_INTEGRITY_BLOCK)
				{
					const dtn::security::SecurityBlock& sb = dynamic_cast<const dtn::security::SecurityBlock&>(*(*iter));
					if ((sb._ciphersuite_flags & SecurityBlock::CONTAINS_CORRELATOR) && sb._correlator == correlator)
						break;
				}
			}

			// consume the first block. this block may have the same correlator set, 
			// we are searching for in the loop
			(*this) << (*(*iter));
			iter++;

			// serialize the remaining block
			for (; iter != list.end(); iter++)
			{
				const dtn::data::Block &b = (*(*iter));
				(*this) << b;

				// until the block with the second correlator is reached
				if (with_correlator && (b.getType() == SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK) || (b.getType() == SecurityBlock::PAYLOAD_INTEGRITY_BLOCK))
				{
					const dtn::security::SecurityBlock& sb = dynamic_cast<const dtn::security::SecurityBlock&>(*(*iter));
					if ((sb._ciphersuite_flags & SecurityBlock::CONTAINS_CORRELATOR) && sb._correlator == correlator)
						break;
				}
			}

			if (type == dtn::security::SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK)
			{
				std::list<const BundleAuthenticationBlock*> babs = bundle.getBlocks<BundleAuthenticationBlock>();
				for (std::list<const BundleAuthenticationBlock*>::const_iterator it = babs.begin(); it != babs.end(); it++)
					(*it)->set_ignore_security_result(false);
			}
			else if (type == dtn::security::SecurityBlock::PAYLOAD_INTEGRITY_BLOCK)
			{
				std::list<const PayloadIntegrityBlock*> babs = bundle.getBlocks<PayloadIntegrityBlock>();
				for (std::list<const PayloadIntegrityBlock*>::const_iterator it = babs.begin(); it != babs.end(); it++)
					(*it)->set_ignore_security_result(false);
			}

			return *this;
		}
	}
}
