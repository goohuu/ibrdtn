#include "ibrdtn/security/StrictSerializer.h"
#include "ibrdtn/security/BundleAuthenticationBlock.h"
#include "ibrdtn/security/PayloadIntegrityBlock.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/Bundle.h"

namespace dtn
{
	namespace security
	{
		StrictSerializer::StrictSerializer(std::ostream& stream, const dtn::security::SecurityBlock::BLOCK_TYPES type, const bool with_correlator, const u_int64_t correlator)
		 : DefaultSerializer(stream), _block_type(type), _with_correlator(with_correlator), _correlator(correlator)
		{
		}

		StrictSerializer::~StrictSerializer()
		{
		}

		dtn::data::Serializer& StrictSerializer::operator<<(const dtn::data::Bundle& bundle)
		{
			// rebuild the dictionary
			rebuildDictionary(bundle);

			// serialize the primary block
			(dtn::data::DefaultSerializer&)(*this) << static_cast<const dtn::data::PrimaryBlock&>(bundle);

			// serialize all secondary blocks
			std::list<refcnt_ptr<dtn::data::Block> > list = bundle._blocks._blocks;
			std::list<refcnt_ptr<dtn::data::Block> >::const_iterator iter = list.begin();

			// skip all blocks before the correlator
			for (; _with_correlator && iter != list.end(); iter++)
			{
				const dtn::data::Block &b = (*(*iter));
				if (b.getType() == SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK || b.getType() == SecurityBlock::PAYLOAD_INTEGRITY_BLOCK)
				{
					const dtn::security::SecurityBlock& sb = dynamic_cast<const dtn::security::SecurityBlock&>(*(*iter));
					if ((sb._ciphersuite_flags & SecurityBlock::CONTAINS_CORRELATOR) && sb._correlator == _correlator)
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

				try {
					const dtn::security::SecurityBlock &sb = dynamic_cast<const dtn::security::SecurityBlock&>(b);

					if ( (sb.getType() == SecurityBlock::BUNDLE_AUTHENTICATION_BLOCK) || (sb.getType() == SecurityBlock::PAYLOAD_INTEGRITY_BLOCK) )
					{
						// until the block with the second correlator is reached
						if (_with_correlator && (sb._ciphersuite_flags & SecurityBlock::CONTAINS_CORRELATOR) && sb._correlator == _correlator) break;
					}
				} catch (const std::bad_cast&) { };
			}

			return *this;
		}

		dtn::data::Serializer& StrictSerializer::operator<<(const dtn::data::Block &obj)
		{
			_stream << obj._blocktype;
			_stream << dtn::data::SDNV(obj._procflags);

#ifdef __DEVELOPMENT_ASSERTIONS__
			// test: BLOCK_CONTAINS_EIDS => (_eids.size() > 0)
			assert(!obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS) || (obj._eids.size() > 0));
#endif

			if (obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS))
			{
				_stream << dtn::data::SDNV(obj._eids.size());
				for (std::list<dtn::data::EID>::const_iterator it = obj._eids.begin(); it != obj._eids.end(); it++)
				{
					pair<size_t, size_t> offsets;

					if (_compressable)
					{
						offsets = (*it).getCompressed();
					}
					else
					{
						offsets = _dictionary.getRef(*it);
					}

					_stream << dtn::data::SDNV(offsets.first);
					_stream << dtn::data::SDNV(offsets.second);
				}
			}

			// write size of the payload in the block
			_stream << dtn::data::SDNV(obj.getLength_strict());

			size_t slength = 0;
			obj.serialize_strict(_stream, slength);

			return (*this);
		}
	}
}
