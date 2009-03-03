#include "data/BundleFactory.h"
#include "data/SDNV.h"
#include "data/PayloadBlock.h"
#include "data/PayloadBlockFactory.h"
#include "data/Dictionary.h"
#include "data/BlockFlags.h"
#include "data/CustodySignalBlock.h"
#include "data/StatusReportBlock.h"
#include <stdio.h>
#include <cstdlib>
#include <cstring>
#include <limits.h>
#include <list>
#include <stdexcept>

namespace dtn
{
	namespace data
	{
		unsigned int BundleFactory::SECONDS_TILL_2000 = 946681200;

		BundleFactory& BundleFactory::getInstance()
		{
			static BundleFactory instance;
			return instance;
		}

		BundleFactory::BundleFactory()
		{
			PayloadBlockFactory *f = new PayloadBlockFactory();
			registerExtensionBlock(f);
		}

		BundleFactory::~BundleFactory()
		{
			BlockFactory *f = m_extensions.at(PayloadBlock::BLOCK_TYPE);
			unregisterExtensionBlock(f);
			delete f;
		}

		void BundleFactory::registerExtensionBlock(BlockFactory *f)
		{
			m_extensions[f->getBlockType()] = f;
		}

		void BundleFactory::unregisterExtensionBlock(BlockFactory *f)
		{
			m_extensions.erase(f->getBlockType());
		}

		BlockFactory& BundleFactory::getExtension(unsigned char type)
		{
			try {
				BlockFactory *bf = m_extensions.at(type);
				return *bf;
			} catch (std::out_of_range ex) {
				return m_blockfactory;
			}
		}

		map<char, BlockFactory*>& BundleFactory::getExtensions()
		{
			return m_extensions;
		}

		NetworkFrame* BundleFactory::parsePrimaryBlock(const unsigned char *data, unsigned int size)
		{
			// remind the origin data pointer
			const unsigned char *datap = data;

			// mapping for the field sizes
			map<unsigned int, unsigned int> fieldsizes;

			// size of the parsed data
			size_t parsedsize = 0;

			// block length
			u_int64_t block_length = 0;

			// check the version
			if (data[0] != BUNDLE_VERSION)
			{
				throw InvalidBundleData("Invalid version of the bundle protocol.");
			}

			// jump to the next field
			block_length++;
			fieldsizes[0] = sizeof(char);
			data += sizeof(char);
			parsedsize += sizeof(char);

			// variable to hold the current field size
			size_t field_len = 0;

			// variable for the processing flags
			u_int64_t procflags = 0;

			// variable for the dictionary length
			u_int64_t dict_length = 0;

			// Durchlaufe alle Felder von Proc. Flags bis Directory Length
			for (int i = PROCFLAGS; i <= DICTIONARY_LENGTH; i++)
			{
				// SDNV: Proc. Flags
				field_len = SDNV::len(data);

				// Wenn kein neues Feld gefunden wurde, abbrechen
				if (field_len <= 0)
				{
					throw InvalidBundleData();
				}

				switch (i)
				{
				case BLOCKLENGTH:
				{
					u_int64_t value = 0;
					SDNV::decode(data, field_len, &value);
					block_length += field_len;
					block_length += value;

					// if the remaining available data is smaller than the size
					// in the primary block throw a exception.
					if ( value > (size - (parsedsize + field_len)) )
					{
						throw InvalidBundleData();
					}
					break;
				}
				case PROCFLAGS:
					SDNV::decode(data, field_len, &procflags);
					block_length += field_len;
					break;
				case DICTIONARY_LENGTH:
					SDNV::decode(data, field_len, &dict_length);
					break;
				}

				// Feldlänge merken
				fieldsizes[i] = field_len;

				// Position weiterrücken
				data += field_len;
				parsedsize += field_len;
			}

			// dictionary
			fieldsizes[DICTIONARY_BYTEARRAY] = dict_length;
			data += dict_length;
			parsedsize += dict_length;

			// decode the primary processing flags
			PrimaryFlags flags( procflags );

			if ( flags.isFragment() )
			{
				// Wenn dies ein Fragment ist, dann auch Fragment Offset lesen
				field_len = SDNV::len(data);

				fieldsizes[FRAGMENTATION_OFFSET] = field_len;

				// Position weiterrücken
				data += field_len;

				// ... und die Gesamtgröße
				field_len = SDNV::len(data);

				fieldsizes[APPLICATION_DATA_LENGTH] = field_len;

				// Position weiterrücken
				data += field_len;
				parsedsize += field_len;
			}

			NetworkFrame *frame = new NetworkFrame(datap, block_length);
			frame->setFieldSizeMap(fieldsizes);

			return frame;
		}
		Bundle* BundleFactory::parse(const unsigned char *data, unsigned int size)
		{
			// parse the primary block and get a NetworkFrame object with its data.
			NetworkFrame *frame = parsePrimaryBlock(data, size);

			// move the data pointer to the new position
			size_t primaryblock_size = frame->getSize();
			data += primaryblock_size;
			size -= primaryblock_size;

			// list for the following blocks
			list<Block*> blocks;

			// decode the primary processing flags
			PrimaryFlags flags( frame->getSDNV(PROCFLAGS) );

			while (0 < size)
			{
				Block *block = getBlock(data, size);
				if (block != NULL)
				{
					PayloadBlock *pblock = dynamic_cast<PayloadBlock*>(block);

					// if block is a payload block and admrecord flag is set
					if ((pblock != NULL) && (flags.isAdmRecord()))
					{
						// transform this block to a admrecord block
						block = (Block*)(PayloadBlockFactory::castAdministrativeBlock(pblock));
					}

					// The payload should be an administrative block, but it isn't!
					if (block == NULL)
					{
						throw InvalidBundleData("The payload should be an administrative block, but it isn't!");
					}

					blocks.push_back(block);

					// move to the next block
					size_t block_size = block->getFrame().getSize();
					data += block_size;
					size -= block_size;
				}
				else
					break;
			}

			return new Bundle(frame, blocks);
		}

		Bundle* BundleFactory::newBundle()
		{
			NetworkFrame *frame = new NetworkFrame();

			// Bundleversion hinzufügen
			frame->append( data::BUNDLE_VERSION );

			// Durchlaufen der Felder bis Dictionary ByteArray
			int maxfield = DICTIONARY_BYTEARRAY;

			// Schreibe ProcFlags
			PrimaryFlags procflags;

			procflags.setEIDSingleton(true);
			frame->append( procflags.getValue() );

			// Vorläufige Blocklänge eintragen
			frame->append( (u_int64_t)0 );

			// Positionen der Dictionary Einträge errechnen
			Dictionary dict;

			// Dictionary Offsets schreiben
			for (int i = 0; i < 4; i++)
			{
				pair<int, int> offsets = dict.add( "dtn:none" );

				// Scheme
				frame->append( (u_int64_t)offsets.first );

				// SSP
				frame->append( (u_int64_t)offsets.second );
			}

			unsigned char* dict_data = (unsigned char*)calloc(dict.getLength(), sizeof(unsigned char));
			unsigned int dict_size = dict.write(dict_data);

			// Durchlaufe alle Felder von Proc. Flags bis Directory Length
			for (int i = CREATION_TIMESTAMP; i <= maxfield; i++)
			{
				switch (i)
				{
					case CREATION_TIMESTAMP:
						frame->append( BundleFactory::getDTNTime() );
					break;

					case CREATION_TIMESTAMP_SEQUENCE:
						frame->append( BundleFactory::getSequenceNumber() );
					break;

					case DICTIONARY_LENGTH:
						frame->append( (u_int64_t)dict.getLength() );
					break;

					case DICTIONARY_BYTEARRAY:
						frame->append( dict_data, dict_size );
					break;

					case LIFETIME:
						frame->append( (u_int64_t)3600 );
					break;

					default:
						frame->append( (u_int64_t)0 );
					break;
				}
			}

			free (dict_data);

			Bundle *bundle = new Bundle(frame);
			bundle->updateBlockLength();

			return bundle;
		}

		unsigned int BundleFactory::getDTNTime()
		{
			return time(NULL) - SECONDS_TILL_2000;
		}

		unsigned int BundleFactory::getSequenceNumber()
		{
			static unsigned int sequencenumber;
			sequencenumber++;
			return sequencenumber;
		}

		Block* BundleFactory::getBlock(const unsigned char *data, unsigned int size)
		{
			BundleFactory &fac = BundleFactory::getInstance();
			BlockFactory &f = fac.getExtension(data[0]);
			return f.parse(data, size);
		}

		Bundle* BundleFactory::merge(const Bundle *fragment1, const Bundle *fragment2)
		{
			// PrimaryFlags (Fragment)
			PrimaryFlags flags = fragment1->getPrimaryFlags();
			if (!flags.isFragment())
			{
				throw FragmentationException("At least one of the bundles isn't a fragment.");
			}

			if (!fragment2->getPrimaryFlags().isFragment())
			{
				throw FragmentationException("At least one of the bundles isn't a fragment.");
			}

			// check that they belongs together
			if (( fragment1->getInteger(CREATION_TIMESTAMP) != fragment2->getInteger(CREATION_TIMESTAMP) ) ||
				( fragment1->getInteger(CREATION_TIMESTAMP_SEQUENCE) != fragment2->getInteger(CREATION_TIMESTAMP_SEQUENCE) ) ||
				( fragment1->getInteger(LIFETIME) != fragment2->getInteger(LIFETIME) ) ||
				( fragment1->getInteger(APPLICATION_DATA_LENGTH) != fragment2->getInteger(APPLICATION_DATA_LENGTH) ) ||
				( fragment1->getSource() != fragment2->getSource() ) )
			{
				// exception
				throw FragmentationException("This fragments don't belongs together.");
			}

			// checks complete, now merge the blocks
			PayloadBlock *payload1 = fragment1->getPayloadBlock();
			PayloadBlock *payload2 = fragment2->getPayloadBlock();

			// TODO: copy blocks other than the payload block!

			unsigned int endof1 = fragment1->getInteger(FRAGMENTATION_OFFSET) + payload1->getLength();

			if (endof1 < fragment2->getInteger(FRAGMENTATION_OFFSET))
			{
				// this aren't adjacency fragments
				throw FragmentationException("This aren't adjacency fragments and can't be merged.");
			}

			// relative offset of payload1 to payload2
			unsigned int p2offset = fragment2->getInteger(FRAGMENTATION_OFFSET) - fragment1->getInteger(FRAGMENTATION_OFFSET);

			// merge the payloads
			PayloadBlock *block = PayloadBlockFactory::merge(payload1, payload2, p2offset);

			// get the frame of fragment1 and use it as base
			const NetworkFrame &frame = fragment1->getFrame();

			// copy the frame of fragment1
			NetworkFrame *framecopy = new NetworkFrame(frame);

			// create a new bundle
			Bundle *bundle = new Bundle(framecopy);

			// if the bundle is complete return a non-fragmented bundle instead of the fragment
			if (block->getLength() == fragment1->getInteger(APPLICATION_DATA_LENGTH))
			{
				// remove the fragment fields of the bundle data
				bundle->setFragment(false);
			}

			// append the block to the bundle
			bundle->appendBlock(block);

			return bundle;
		}

		bool BundleFactory::compareFragments(Bundle *first, Bundle *second)
		{
			// Wenn der Offset des aktuellen Bundles kleiner ist als der des einzufügenden Bundles
			if (first->getInteger(FRAGMENTATION_OFFSET) < second->getInteger(FRAGMENTATION_OFFSET))
			{
				return true;
			}
			else
			{
				return false;
			}
		}

		Bundle* BundleFactory::merge(list<Bundle*> &bundles)
		{
			// no bundle, raise a exception
			if (bundles.size() <= 1)
			{
				throw FragmentationException("None or only one item in the list.");
			}

			// sort the fragments
			bundles.sort(BundleFactory::compareFragments);

			// take a copy of the first bundle as base and merge it with the others
			Bundle *first = bundles.front();
			bundles.pop_front();
			Bundle *second = bundles.front();
			Bundle *bundle = NULL;

			try {
				// the first merge creates a new bundle object
				bundle = merge(first, second);
				bundles.pop_front();

				delete first;
				delete second;
			} catch (FragmentationException ex) {
				bundles.push_front(first);
				throw ex;
			}

			PrimaryFlags flags = bundle->getPrimaryFlags();
			if (flags.isFragment())
			{
				// put the new fragment into the list
				bundles.push_back(bundle);

				// call merge recursive
				return merge(bundles);
			}
			else
			{
				return bundle;
			}
		}

		pair<Bundle*, Bundle*> BundleFactory::cutAt(const Bundle *bundle, unsigned int position)
		{
			// Get the payload of the bundle.
			PayloadBlock *payload = bundle->getPayloadBlock();

			// Raise a exception if no payloadblock exists.
			if (payload == NULL)
			{
				throw FragmentationException("Fragmentation not possible. No payload block exists.");
			}

			// get the range of the payload block
			pair<unsigned int, unsigned int> range = payload->getPayloadRange();

			// fragmentation is not possible, if the position is not in the payload
			if ((position < range.first) && (position >= range.second))
			{
				throw FragmentationException("Fragmentation not possible. The suggested cut is not in the payload.");
			}

			// determine the cut position in the payload data
			unsigned int size = position - range.first;

			// offset parameter
			unsigned int offset = 0;

			Bundle *b1 = BundleFactory::cut(bundle, size, offset);
			Bundle *b2 = BundleFactory::cut(bundle, UINT_MAX, offset);

			return make_pair(b1, b2);
		}

		Bundle* BundleFactory::cut(const Bundle *bundle, unsigned int size, unsigned int& offset)
		{
			// Read the primary flags of the bundle.
			PrimaryFlags flags = bundle->getPrimaryFlags();

			// If fragmentation is forbidden, the just throw a exception.
			if (flags.isFragmentationForbidden())
			{
				throw DoNotFragmentBitSetException();
			}

			// Get the payload of the bundle.
			PayloadBlock *payload = bundle->getPayloadBlock();

			// Raise a exception if no payloadblock exists.
			if (payload == NULL)
			{
				throw FragmentationException("Fragmentation not possible. No payload block exists.");
			}

			// If the offset is bigger than the payload, return NULL.
			if (offset >= (unsigned int)payload->getLength()) return NULL;

			// Get the data of the payload block.
			unsigned char *data = payload->getPayload();
			data += offset;

			if (size == UINT_MAX)
			{
				// Max length is requested. Take the hole remaining data.
				size = payload->getLength() - offset;
			}

			// get the frame of fragment1 and use it as base
			const NetworkFrame &frame = bundle->getFrame();

			// copy the frame of bundle
			NetworkFrame *framecopy = new NetworkFrame(frame);

			// create a new bundle
			dtn::data::Bundle *fragment = new Bundle(framecopy);

			// set the new bundle to a fragment
			fragment->setFragment(true);

			if (flags.isFragment())
			{
				fragment->setInteger( FRAGMENTATION_OFFSET,
						bundle->getInteger(FRAGMENTATION_OFFSET) + offset );
			}
			else
			{
				// set the offset of the new fragment
				fragment->setInteger( FRAGMENTATION_OFFSET, offset );
				fragment->setInteger( APPLICATION_DATA_LENGTH, payload->getLength() );
			}

			// create a payload block
			PayloadBlock *fragment_block = PayloadBlockFactory::newPayloadBlock(data, size);

			// add the payload block to the fragment
			fragment->appendBlock(fragment_block);

			// change the offset
			offset += size;

			return fragment;
		}

		Bundle* BundleFactory::slice(const Bundle *bundle, unsigned int size, unsigned int& offset)
		{
			// Read the primary flags of the bundle.
			PrimaryFlags flags = bundle->getPrimaryFlags();

			// If fragmentation is forbidden, the just throw a exception.
			if (flags.isFragmentationForbidden())
			{
				throw DoNotFragmentBitSetException();
			}

			// Get the payload of the bundle.
			PayloadBlock *payload = bundle->getPayloadBlock();

			// Raise a exception if no payloadblock exists.
			if (payload == NULL)
			{
				throw FragmentationException("Fragmentation not possible. No payload block exists.");
			}

			// Get the data of the payload block.
			unsigned char *data = payload->getPayload();
			data += offset;

			// If the offset is bigger than the payload, return NULL.
			if (offset >= (unsigned int)payload->getLength()) return NULL;

			if (size == UINT_MAX)
			{
				// Max length is requested. Take the hole remaining data.
				size = payload->getLength() - offset;
			}
			else
			{
				// Overhead berechnen und von der Zielgroße abziehen
				size -= bundle->getLength() - payload->getLength();

				if (!flags.isFragment())
				{
					size -= SDNV::encoding_len( (u_int64_t)offset);
					size -= SDNV::encoding_len( (u_int64_t)payload->getLength() );
				}

				// Wenn Restdatenlänge kürzer als size, dann Restlänge berechnen
				if ((offset + size) > (unsigned int)payload->getLength())
				{
					size = payload->getLength() - offset;
				}
			}

			// get the frame of fragment1 and use it as base
			const NetworkFrame &frame = bundle->getFrame();

			// copy the frame of bundle
			NetworkFrame *framecopy = new NetworkFrame(frame);

			// create a new bundle
			dtn::data::Bundle *fragment = new Bundle(framecopy);

			// set the new bundle to a fragment
			fragment->setFragment(true);

			// set the new bundle to a fragment
			fragment->setFragment(true);

			if (flags.isFragment())
			{
				fragment->setInteger( FRAGMENTATION_OFFSET,
						bundle->getInteger(FRAGMENTATION_OFFSET) + offset );
			}
			else
			{
				// set the offset of the new fragment
				fragment->setInteger( FRAGMENTATION_OFFSET, offset );
				fragment->setInteger( APPLICATION_DATA_LENGTH, payload->getLength() );
			}

			// create a payload block
			PayloadBlock *fragment_block = PayloadBlockFactory::newPayloadBlock(data, size);

			// add the payload block to the fragment
			fragment->appendBlock(fragment_block);

			// change the offset
			offset += size;

			return fragment;
		}

		list<Bundle*> BundleFactory::split(const Bundle *bundle, unsigned int maxsize)
		{
			list<Bundle*> ret;
			unsigned int offset = 0;

			Bundle *fragment = BundleFactory::slice(bundle, maxsize, offset);

			while (fragment != NULL)
			{
				ret.push_back(fragment);
				fragment = BundleFactory::slice(bundle, maxsize, offset);
			}

			return ret;
		}
	}
}

