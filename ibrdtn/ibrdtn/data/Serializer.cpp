#include "ibrdtn/data/Serializer.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/ExtensionBlock.h"
#include "ibrdtn/data/ExtensionBlockFactory.h"
#include "ibrcommon/refcnt_ptr.h"
#include <list>
#include <cassert>

namespace dtn
{
    namespace data
    {
        DefaultSerializer::DefaultSerializer(std::ostream& stream)
         : _stream(stream)
        {

        }

        DefaultSerializer::DefaultSerializer(std::ostream& stream, const Dictionary &d)
         : _stream(stream), _dictionary(d)
        {

        }

		void DefaultSerializer::rebuildDictionary(const dtn::data::Bundle &obj)
		{
			// clear the dictionary
			_dictionary.clear();

			// rebuild the dictionary
			_dictionary.add(obj._destination);
			_dictionary.add(obj._source);
			_dictionary.add(obj._reportto);
			_dictionary.add(obj._custodian);

            // add EID of all secondary blocks
			std::list<refcnt_ptr<Block> > list = obj._blocks._blocks;

			for (std::list<refcnt_ptr<Block> >::const_iterator iter = list.begin(); iter != list.end(); iter++)
			{
				const Block &b = (*(*iter));
				_dictionary.add( b.getEIDList() );
			}
		}

        Serializer& DefaultSerializer::operator <<(const dtn::data::Bundle& obj)
        {
			// rebuild the dictionary
			rebuildDictionary(obj);

            // serialize the primary block
            (*this) << (PrimaryBlock&)obj;

            // serialize all secondary blocks
			std::list<refcnt_ptr<Block> > list = obj._blocks._blocks;
			
			for (std::list<refcnt_ptr<Block> >::const_iterator iter = list.begin(); iter != list.end(); iter++)
			{
				const Block &b = (*(*iter));
				(*this) << b;
			}

            return (*this);
        }

		Serializer& DefaultSerializer::operator <<(const dtn::data::PrimaryBlock& obj)
		{
			_stream << dtn::data::BUNDLE_VERSION;		// bundle version
			_stream << dtn::data::SDNV(obj._procflags);	// processing flags

			// predict the block length
			size_t len = 0;
			dtn::data::SDNV primaryheader[14];
			pair<size_t, size_t> ref;

			// destination reference
			ref = _dictionary.getRef(obj._destination);
			primaryheader[0] = SDNV(ref.first);
			primaryheader[1] = SDNV(ref.second);

			// source reference
			ref = _dictionary.getRef(obj._source);
			primaryheader[2] = SDNV(ref.first);
			primaryheader[3] = SDNV(ref.second);

			// reportto reference
			ref = _dictionary.getRef(obj._reportto);
			primaryheader[4] = SDNV(ref.first);
			primaryheader[5] = SDNV(ref.second);

			// custodian reference
			ref = _dictionary.getRef(obj._custodian);
			primaryheader[6] = SDNV(ref.first);
			primaryheader[7] = SDNV(ref.second);

			// timestamp
			primaryheader[8] = SDNV(obj._timestamp);

			// sequence number
			primaryheader[9] = SDNV(obj._sequencenumber);

			// lifetime
			primaryheader[10] = SDNV(obj._lifetime);

			// dictionary size
			primaryheader[11] = SDNV(_dictionary.getSize());

			for (int i = 0; i < 12; i++)
			{
				len += primaryheader[i].getLength();
			}

			len += _dictionary.getSize();

			if (obj._procflags & dtn::data::Bundle::FRAGMENT)
			{
				primaryheader[12] = SDNV(obj._fragmentoffset);
				primaryheader[13] = SDNV(obj._appdatalength);

				len += primaryheader[12].getLength();
				len += primaryheader[13].getLength();
			}

			// write the block length
			_stream << SDNV(len);

			/*
			 * write the ref block of the dictionary
			 * this includes scheme and ssp for destination, source, reportto and custodian.
			 */
			for (int i = 0; i < 12; i++)
			{
				_stream << primaryheader[i];
			}

			_stream << _dictionary; // DICTIONARY_BYTEARRAY

			if (obj._procflags & dtn::data::Bundle::FRAGMENT)
			{
				_stream << primaryheader[12]; // FRAGMENTATION_OFFSET
				_stream << primaryheader[13]; // APPLICATION_DATA_LENGTH
			}
			
			return (*this);
		}

        Serializer& DefaultSerializer::operator <<(const dtn::data::Block& obj)
        {
			_stream << obj._blocktype;
			_stream << dtn::data::SDNV(obj._procflags);

			// test: BLOCK_CONTAINS_EIDS => (_eids.size() > 0)
			assert(!(obj._procflags & Block::BLOCK_CONTAINS_EIDS) || (obj._eids.size() > 0));

			if (obj._procflags & Block::BLOCK_CONTAINS_EIDS)
			{
				_stream << SDNV(obj._eids.size());
				for (std::list<dtn::data::EID>::const_iterator it = obj._eids.begin(); it != obj._eids.end(); it++)
				{
					pair<size_t, size_t> offsets = _dictionary.getRef(*it);
					_stream << SDNV(offsets.first);
					_stream << SDNV(offsets.second);
				}
			}

			// write size of the payload in the block
			_stream << SDNV(obj.getLength());

			// write the payload of the block
			obj.serialize(_stream);

            return (*this);
        }

        const size_t DefaultSerializer::getLength(const dtn::data::Bundle &obj) const
        {
			// check if the dictionary is empty
			assert(_dictionary.getSize() > 0);

			size_t len = 0;
			len += getLength( (PrimaryBlock&)obj );
			
			// add size of all blocks
			std::list<refcnt_ptr<Block> > list = obj._blocks._blocks;

			for (std::list<refcnt_ptr<Block> >::const_iterator iter = list.begin(); iter != list.end(); iter++)
			{
				const Block &b = (*(*iter));
				len += getLength( b );
			}

			return len;
        }

		const size_t DefaultSerializer::getLength(const dtn::data::PrimaryBlock& obj) const
		{
			size_t len = 0;

			len += sizeof(dtn::data::BUNDLE_VERSION);		// bundle version
			len += dtn::data::SDNV(obj._procflags).getLength();	// processing flags

			// primary header
			dtn::data::SDNV primaryheader[14];
			pair<size_t, size_t> ref;

			// destination reference
			ref = _dictionary.getRef(obj._destination);
			primaryheader[0] = SDNV(ref.first);
			primaryheader[1] = SDNV(ref.second);

			// source reference
			ref = _dictionary.getRef(obj._source);
			primaryheader[2] = SDNV(ref.first);
			primaryheader[3] = SDNV(ref.second);

			// reportto reference
			ref = _dictionary.getRef(obj._reportto);
			primaryheader[4] = SDNV(ref.first);
			primaryheader[5] = SDNV(ref.second);

			// custodian reference
			ref = _dictionary.getRef(obj._custodian);
			primaryheader[6] = SDNV(ref.first);
			primaryheader[7] = SDNV(ref.second);

			// timestamp
			primaryheader[8] = SDNV(obj._timestamp);

			// sequence number
			primaryheader[9] = SDNV(obj._sequencenumber);

			// lifetime
			primaryheader[10] = SDNV(obj._lifetime);

			// dictionary size
			primaryheader[11] = SDNV(_dictionary.getSize());

			for (int i = 0; i < 12; i++)
			{
				len += primaryheader[i].getLength();
			}

			len += _dictionary.getSize();

			if (obj._procflags & dtn::data::Bundle::FRAGMENT)
			{
				primaryheader[12] = SDNV(obj._fragmentoffset);
				primaryheader[13] = SDNV(obj._appdatalength);

				len += primaryheader[12].getLength();
				len += primaryheader[13].getLength();
			}

			return len;
		}

        const size_t DefaultSerializer::getLength(const dtn::data::Block &obj) const
        {
			size_t len = 0;

			len += sizeof(obj._blocktype);
			len += dtn::data::SDNV(obj._procflags).getLength();

			// test: BLOCK_CONTAINS_EIDS => (_eids.size() > 0)
			assert(!(obj._procflags & Block::BLOCK_CONTAINS_EIDS) || (obj._eids.size() > 0));

			if (obj._procflags & Block::BLOCK_CONTAINS_EIDS)
			{
				len += dtn::data::SDNV(obj._eids.size()).getLength();
				for (std::list<dtn::data::EID>::const_iterator it = obj._eids.begin(); it != obj._eids.end(); it++)
				{
					pair<size_t, size_t> offsets = _dictionary.getRef(*it);
					len += SDNV(offsets.first).getLength();
					len += SDNV(offsets.second).getLength();
				}
			}

			// size of the payload in the block
			len += obj.getLength();

			return len;
        }

        DefaultDeserializer::DefaultDeserializer(std::istream& stream)
         : _stream(stream)
        {
        }

		DefaultDeserializer::DefaultDeserializer(std::istream &stream, const Dictionary &d)
		 : _stream(stream), _dictionary(d)
		{
		}

        Deserializer& DefaultDeserializer::operator >>(dtn::data::Bundle& obj)
        {
            (*this) >> (PrimaryBlock&)obj;

            // read until the last block
            bool lastblock = false;

            try {
				// read all BLOCKs
				while (!_stream.eof() && !lastblock)
				{
					char block_type;
					size_t block_flags = 0;
					size_t block_length = 0;

					// BLOCK_TYPE
					block_type = _stream.peek();

					switch (block_type)
					{
						case 0:
						{
							throw dtn::exceptions::InvalidDataException("block type is zero");
							break;
						}

						case dtn::data::PayloadBlock::BLOCK_TYPE:
						{
							if (obj._procflags & dtn::data::Bundle::APPDATA_IS_ADMRECORD)
							{
								// create a temporary block
								dtn::data::ExtensionBlock &block = obj.appendBlock<dtn::data::ExtensionBlock>();

								// read the block data
								(*this) >> block;

								// access the payload to get the first byte
								ibrcommon::BLOB::Reference ref = block.getBLOB();
								char admfield; (*ref) >> admfield;

								// write the block into a temporary stream
								stringstream ss;
								DefaultSerializer serializer(ss, _dictionary);
								DefaultDeserializer deserializer(ss, _dictionary);

								serializer << block;

								// remove the temporary block
								obj.removeBlock(block);

								switch (admfield >> 4)
								{
									case 1:
									{
										dtn::data::StatusReportBlock &block = obj.appendBlock<dtn::data::StatusReportBlock>();
										deserializer >> block;
										lastblock = block.get(Block::LAST_BLOCK);
										break;
									}

									case 2:
									{
										dtn::data::CustodySignalBlock &block = obj.appendBlock<dtn::data::CustodySignalBlock>();
										deserializer >> block;
										lastblock = block.get(Block::LAST_BLOCK);
										break;
									}

									default:
									{
										// drop unknown administrative block
										break;
									}
								}

							}
							else
							{
								dtn::data::PayloadBlock &block = obj.appendBlock<dtn::data::PayloadBlock>();
								(*this) >> block;

								lastblock = block.get(Block::LAST_BLOCK);
							}
							break;
						}

						default:
						{
							// get a extension block factory
							std::map<char, ExtensionBlockFactory*> &factories = dtn::data::Bundle::getExtensionBlockFactories();
							std::map<char, ExtensionBlockFactory*>::iterator iter = factories.find(block_type);

							if (iter != factories.end())
							{
								ExtensionBlockFactory &f = (*iter->second);
								dtn::data::Block &block = obj.appendBlock(f);
								(*this) >> block;
								lastblock = block.get(Block::LAST_BLOCK);
							}
							else
							{
								dtn::data::ExtensionBlock &block = obj.appendBlock<dtn::data::ExtensionBlock>();
								(*this) >> block;
								lastblock = block.get(Block::LAST_BLOCK);
							}
							break;
						}
					}
				}
			} catch (dtn::exceptions::InvalidDataException ex) {
				// end of bundle
			} catch (dtn::exceptions::IOException ex) {
				// read aborted. let the endBundle-Method check if there is enough
				// data for a fragment.
			}
            
            return (*this);
        }

		Deserializer& DefaultDeserializer::operator >>(dtn::data::PrimaryBlock& obj)
		{
			char version = 0;
			SDNV tmpsdnv;
			SDNV blocklength;

			// check for the right version
			_stream >> version;
			if (version != dtn::data::BUNDLE_VERSION) throw exceptions::InvalidBundleData("Bundle version differ from ours.");

			// PROCFLAGS
			_stream >> tmpsdnv;	// processing flags
			obj._procflags = tmpsdnv.getValue();

			// BLOCK LENGTH
			_stream >> blocklength;

			// EID References
			pair<SDNV, SDNV> ref[4];
			for (int i = 0; i < 4; i++)
			{
				_stream >> ref[i].first;
				_stream >> ref[i].second;
			}

			// timestamp
			_stream >> tmpsdnv;
			obj._timestamp = tmpsdnv.getValue();

			// sequence number
			_stream >> tmpsdnv;
			obj._sequencenumber = tmpsdnv.getValue();

			// lifetime
			_stream >> tmpsdnv;
			obj._lifetime = tmpsdnv.getValue();

			// dictionary size
			_stream >> tmpsdnv;

			// dictionary
			char *data = (char*)calloc(tmpsdnv.getValue(), sizeof(char));
			_stream.read(data, tmpsdnv.getValue());
			_dictionary = Dictionary(data, tmpsdnv.getValue());

			// decode EIDs
			obj._destination = _dictionary.get(ref[0].first.getValue(), ref[0].second.getValue());
			obj._source = _dictionary.get(ref[1].first.getValue(), ref[1].second.getValue());
			obj._reportto = _dictionary.get(ref[2].first.getValue(), ref[2].second.getValue());
			obj._custodian = _dictionary.get(ref[3].first.getValue(), ref[3].second.getValue());

			// fragmentation?
			if (obj._procflags & dtn::data::Bundle::FRAGMENT)
			{
				_stream >> tmpsdnv;
				obj._fragmentoffset = tmpsdnv.getValue();

				_stream >> tmpsdnv;
				obj._appdatalength = tmpsdnv.getValue();
			}
			
			return (*this);
		}

        Deserializer&  DefaultDeserializer::operator >>(dtn::data::Block& obj)
        {
			dtn::data::SDNV procflags_sdnv;
			_stream >> obj._blocktype;
			_stream >> procflags_sdnv;
			obj._procflags = procflags_sdnv.getValue();

			// TODO: read EIDs!

			// read the size of the payload in the block
			SDNV block_size;
			_stream >> block_size;
			obj._blocksize = block_size.getValue();

			// read the payload of the block
			obj.deserialize(_stream);
			
            return (*this);
        }
    }
}
