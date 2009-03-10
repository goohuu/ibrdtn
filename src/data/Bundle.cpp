#include "data/Bundle.h"
#include "data/SDNV.h"
#include "data/PrimaryFlags.h"
#include "data/BundleFactory.h"
#include "data/StatusReportBlock.h"
#include "data/CustodySignalBlock.h"
#include <sstream>
#include <stdio.h>
#include <cstdlib>
#include <cstring>

using namespace std;

namespace dtn
{
	namespace data
	{
		Bundle::Bundle(NetworkFrame *frame, const list<Block*> blocks) : m_frame(frame), m_blocks(blocks)
		{
		}

		Bundle::Bundle(NetworkFrame *frame) : m_frame(frame)
		{
		}

		Bundle::Bundle(const Bundle &b) : m_frame(NULL)
		{
			m_frame = new NetworkFrame(b.m_frame);

			// copy all blocks
			list<Block*> blocks = b.m_blocks;
			list<Block*>::const_iterator iter = blocks.begin();

			BundleFactory &fac = BundleFactory::getInstance();

			while (iter != blocks.end())
			{
				m_blocks.push_back( fac.copyBlock( *(*iter) ) );
				iter++;
			}
		}

		Bundle::~Bundle()
		{
			// iterate through all blocks
			list<Block*>::const_iterator iter = m_blocks.begin();

			while (iter != m_blocks.end())
			{
				delete (*iter);
				iter++;
			}

			delete m_frame;
		}

		unsigned int Bundle::getLength() const
		{
			unsigned int size = 0;

			// add the size of the primary block
			size += m_frame->getSize();

			// iterate through all blocks
			list<Block*>::const_iterator iter = m_blocks.begin();

			while (iter != m_blocks.end())
			{
				size += (*iter)->getFrame().getSize();
				iter++;
			}

			return size;
		}

		unsigned char* Bundle::getData() const
		{
			// get the length of the data
			unsigned int length = getLength();

			// create a new data-array with enough space
			unsigned char *data = (unsigned char*)calloc(length, sizeof(char));
			unsigned char *ret = data;

			// copy the data of the primary block
			memcpy(data, m_frame->getData(), m_frame->getSize());
			data += m_frame->getSize();

			// iterate through all blocks
			list<Block*>::const_iterator iter = m_blocks.begin();

			while (iter != m_blocks.end())
			{
				// Update the block size
				(*iter)->updateBlockSize();

				NetworkFrame &frame = (*iter)->getFrame();

				// copy the data of the block
				memcpy(data, frame.getData(), frame.getSize());

				data += frame.getSize();
				iter++;
			}

			return ret;
		}

		void Bundle::updateBlockLength()
		{
			// Berechne die Blocklänge, alle Felder zwischen
			// DESTINATION_SCHEME und DICTIONARY_BYTEARRAY bzw. APPLICATION_DATA_LENGTH
			unsigned int length = 0;
			unsigned int maxfield = DICTIONARY_BYTEARRAY;

			PrimaryFlags primflags( getInteger( PROCFLAGS ) );
			if (primflags.isFragment()) maxfield = APPLICATION_DATA_LENGTH;

			for (unsigned int i = DESTINATION_SCHEME; i <= maxfield; i++)
			{
				length += m_frame->getSize(i);
			}

			m_frame->set(BLOCKLENGTH, length);
		}

		PrimaryFlags Bundle::getPrimaryFlags() const
		{
			PrimaryFlags flags( getInteger(PROCFLAGS) );
			return flags;
		}

		void Bundle::setPrimaryFlags(PrimaryFlags &flags)
		{
			setInteger( PROCFLAGS, flags.getValue() );
		}

		u_int64_t Bundle::getInteger(const BUNDLE_FIELDS field) const throw (InvalidFieldException, FieldDoesNotExist)
		{
			// Überprüfen ob das angeforderte Feld überhaupt als Integer zurückgegeben werden kann.
			if ( (field == VERSION) ||
				 (field == DICTIONARY_BYTEARRAY) )
			{
				throw InvalidFieldException("Das angegebene Feld ist für diese Operation nicht gültig.");
			}

			// Wert zurückgeben
			return m_frame->getSDNV(field);
		}

		void Bundle::setInteger(BUNDLE_FIELDS field, u_int64_t value) throw (InvalidFieldException, FieldDoesNotExist)
		{
			// Überprüfen ob das angeforderte Feld überhaupt als Integer zurückgegeben werden kann.
			if ( (field == VERSION) ||
				 (field == DICTIONARY_BYTEARRAY) )
			{
				throw InvalidFieldException("Das angegebene Feld ist für diese Operation nicht gültig.");
			}

			// Wert schreiben
			m_frame->set(field, value);

			updateBlockLength();
		}

		int Bundle::getPosition(BUNDLE_FIELDS field) throw (FieldDoesNotExist, InvalidBundleData)
		{
			return m_frame->getPosition(field);
		}

		Dictionary Bundle::getDictionary() const
		{
			Dictionary dict;

			// Dictionary neu lesen
			dict.read( m_frame->get(DICTIONARY_BYTEARRAY), m_frame->getSDNV(DICTIONARY_LENGTH) );

			// Referenz zurückgeben
			return dict;
		}

		void Bundle::commitDictionary(const Dictionary &dict)
		{
			// change the size of the field
			m_frame->changeSize(DICTIONARY_BYTEARRAY, dict.getLength());

			// get the pointer to the dict bytearray
			unsigned char *data = m_frame->get(DICTIONARY_BYTEARRAY);

			// write the dictionary length
			m_frame->set(DICTIONARY_LENGTH, dict.getLength());

			// write data
			dict.write(data);

			updateBlockLength();
		}

		string Bundle::getDestination() const
		{
			const Dictionary &dict = getDictionary();
			return dict.get( getInteger(DESTINATION_SCHEME) ) + ":" + dict.get( getInteger(DESTINATION_SSP) );
		}

		string Bundle::getSource() const
		{
			const Dictionary &dict = getDictionary();
			return dict.get( getInteger(SOURCE_SCHEME) ) + ":" + dict.get( getInteger(SOURCE_SSP) );
		}

		string Bundle::getReportTo() const
		{
			const Dictionary &dict = getDictionary();
			return dict.get( getInteger(REPORTTO_SCHEME) ) + ":" + dict.get( getInteger(REPORTTO_SSP) );
		}

		string Bundle::getCustodian() const
		{
			const Dictionary &dict = getDictionary();
			return dict.get( getInteger(CUSTODIAN_SCHEME) ) + ":" + dict.get( getInteger(CUSTODIAN_SSP) );
		}

		void Bundle::setDestination(string destination)
		{
			Dictionary dict = getDictionary();
			pair<unsigned int, unsigned int> positions = dict.add( destination );

			m_frame->set( (unsigned int)DESTINATION_SCHEME, positions.first );
			m_frame->set( (unsigned int)DESTINATION_SSP, positions.second );

			commitDictionary(dict);
		}

		void Bundle::setSource(string source)
		{
			Dictionary dict = getDictionary();
			pair<unsigned int, unsigned int> positions = dict.add( source );

			m_frame->set( (unsigned int)SOURCE_SCHEME, positions.first );
			m_frame->set( (unsigned int)SOURCE_SSP, positions.second );

			commitDictionary(dict);
		}

		void Bundle::setReportTo(string reportto)
		{
			Dictionary dict = getDictionary();
			pair<unsigned int, unsigned int> positions = dict.add( reportto );

			m_frame->set( (unsigned int)REPORTTO_SCHEME, positions.first );
			m_frame->set( (unsigned int)REPORTTO_SSP, positions.second );

			commitDictionary(dict);
		}

		void Bundle::setCustodian(string custodian)
		{
			Dictionary dict = getDictionary();
			pair<unsigned int, unsigned int> positions = dict.add( custodian );

			m_frame->set( (unsigned int)CUSTODIAN_SCHEME, positions.first );
			m_frame->set( (unsigned int)CUSTODIAN_SSP, positions.second );

			commitDictionary(dict);
		}

		list<Block*> Bundle::getBlocks(const unsigned char type) const
		{
			list<Block*>::const_iterator iter = m_blocks.begin();
			list<Block*> ret;

			while (iter != m_blocks.end())
			{
				Block *block = (*iter);
				if (block->getType() == type)
				{
					ret.push_back(block);
				}

				iter++;
			}

			return ret;
		}

		PayloadBlock* Bundle::getPayloadBlock() const
		{
			list<Block*>::const_iterator iter = m_blocks.begin();

			while (iter != m_blocks.end())
			{
				PayloadBlock *payload = dynamic_cast<PayloadBlock*>(*iter);
				if (payload != NULL) return payload;

				iter++;
			}

			return NULL;
		}

		const list<Block*>& Bundle::getBlocks() const
		{
			return m_blocks;
		}

		void Bundle::appendBlock(Block *block)
		{
			if (m_blocks.size() > 0)
			{
				// get the last block
				Block *last = m_blocks.back();

				// set the last block flag to false
				BlockFlags flags = last->getBlockFlags();
				flags.setFlag(LAST_BLOCK, false);
				last->setBlockFlags(flags);
			}

			// set the last block flag to true
			BlockFlags flags = block->getBlockFlags();
			flags.setFlag(LAST_BLOCK, true);
			block->setBlockFlags(flags);

			// add the new block to the end
			m_blocks.push_back(block);

			// if this is a administrative block, set the required flags
			AdministrativeBlock *admblock = dynamic_cast<AdministrativeBlock*>(block);

			if (admblock != NULL)
			{
				PrimaryFlags pflags = getPrimaryFlags();
				pflags.setAdmRecord(true);
				setPrimaryFlags(pflags);
			}
		}

		void Bundle::insertBlock(Block *block)
		{
			m_blocks.push_front(block);

			// if this is a administrative block, set the required flags
			AdministrativeBlock *admblock = dynamic_cast<AdministrativeBlock*>(block);

			if (admblock != NULL)
			{
				PrimaryFlags pflags = getPrimaryFlags();
				pflags.setAdmRecord(true);
				setPrimaryFlags(pflags);
			}
		}

		void Bundle::removeBlock(Block *block)
		{
			// remove the block
			m_blocks.remove(block);

			if (!m_blocks.empty())
			{
				// get the last block
				Block *last = m_blocks.back();

				// maybe we removed the last block, so set the last block flag to true
				BlockFlags flags = last->getBlockFlags();
				flags.setFlag(LAST_BLOCK, true);
				last->setBlockFlags(flags);
			}
		}

		const NetworkFrame& Bundle::getFrame() const
		{
			return *m_frame;
		}

		void Bundle::setFragment(bool value)
		{
			PrimaryFlags flags = this->getPrimaryFlags();

			// nothing to do, if this is still a fragmented/non-fragmented bundle
			if (flags.isFragment() == value) return;

			if (value)
			{
				flags.setFragment(true);

				// add the fragmentation fields
				m_frame->append(0); // FRAGMENTATION_OFFSET
				m_frame->append(0); // APPLICATION_DATA_LENGTH
			}
			else
			{
				flags.setFragment(false);

				// remove the fragmentation fields
				m_frame->remove(APPLICATION_DATA_LENGTH);
				m_frame->remove(FRAGMENTATION_OFFSET);
			}

			// set the primary flags
			this->setPrimaryFlags(flags);

			updateBlockLength();
		}

		bool Bundle::operator!=(const Bundle& other) const
		{
			if (getSource() == other.getSource()) return false;
			if (getInteger(CREATION_TIMESTAMP) == other.getInteger(CREATION_TIMESTAMP)) return false;
			if (getInteger(CREATION_TIMESTAMP_SEQUENCE) == other.getInteger(CREATION_TIMESTAMP_SEQUENCE)) return false;

			return true;
		}

		bool Bundle::operator==(const Bundle& other) const
		{
			if (getSource() != other.getSource()) return false;
			if (getInteger(CREATION_TIMESTAMP) != other.getInteger(CREATION_TIMESTAMP)) return false;
			if (getInteger(CREATION_TIMESTAMP_SEQUENCE) != other.getInteger(CREATION_TIMESTAMP_SEQUENCE)) return false;

			return true;
		}

		bool Bundle::isExpired() const
		{
			unsigned int time = BundleFactory::getDTNTime();
			return ( (getInteger(LIFETIME) + getInteger(CREATION_TIMESTAMP)) <= time );
		}

		string Bundle::toString() const
		{
			stringstream ss;
			ss << "[" << getInteger(CREATION_TIMESTAMP) << "." << getInteger(CREATION_TIMESTAMP_SEQUENCE);
			string ret; ss >> ret;
			ret.append(" ");
			ret.append(getSource());
			ret.append(" -> ");
			ret.append(getDestination());
			ret.append("]");
			return ret;
		}
	}
}
