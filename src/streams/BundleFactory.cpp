/*
 * BundleFactory.cpp
 *
 *  Created on: 03.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/BundleFactory.h"
#include "ibrdtn/data/BLOBManager.h"
#include "ibrdtn/data/BLOBReference.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/SDNV.h"

#include <stdlib.h>

using namespace dtn::blob;

namespace dtn
{
	namespace streams
	{
		BundleFactory::BundleFactory(BLOBManager &blobmanager)
		 : _blobmanager(blobmanager), _block(NULL), _blockref(-1)
		{
		}

		BundleFactory::~BundleFactory()
		{
			if (_block != NULL) delete _block;
		}

		void BundleFactory::beginBundle(char version)
		{
			_bundle = dtn::data::Bundle();
		}

		void BundleFactory::endBundle()
		{
		}

		void BundleFactory::beginAttribute(ATTRIBUTES attr)
		{
			_next = attr;
		}

		void BundleFactory::endAttribute(char value)
		{

		}

		void BundleFactory::endAttribute(size_t value)
		{
			switch (_next)
			{
				case PROCFLAGS:
					_bundle._procflags = value;
					break;

				case CREATION_TIMESTAMP:
					_bundle._timestamp = value;
					break;

				case CREATION_TIMESTAMP_SEQUENCE:
					_bundle._sequencenumber = value;
					break;

				case LIFETIME:
					_bundle._lifetime = value;
					break;

				case APPLICATION_DATA_LENGTH:
					_bundle._appdatalength = value;
					break;

				case DESTINATION_SCHEME:
					_eids[0].first = value;
					break;

				case DESTINATION_SSP:
					_eids[0].second = value;
					break;

				case SOURCE_SCHEME:
					_eids[1].first = value;
					break;

				case SOURCE_SSP:
					_eids[1].second = value;
					break;

				case REPORTTO_SCHEME:
					_eids[2].first = value;
					break;

				case REPORTTO_SSP:
					_eids[2].second = value;
					break;

				case CUSTODIAN_SCHEME:
					_eids[3].first = value;
					break;

				case CUSTODIAN_SSP:
					_eids[3].second = value;
					break;

				case BLOCK_FLAGS:
					_block->_procflags = value;
					break;

				case BLOCK_REFERENCES:
					// add block references!
					if (_blockref == -1)
					{
						_blockref = value;
					}
					else
					{
						_block->addEID( _dict.get(_blockref, value) );
						_blockref = -1;
					}
					break;

				default:
					break;
			}
		}

		void BundleFactory::endAttribute(char *data, size_t size)
		{
			switch (_next)
			{
				case DICTIONARY_BYTEARRAY:
				{
					//_dict = Dictionary(data, size);
					_dict.read(data, size);
					_bundle._destination = _dict.get(_eids[0].first, _eids[0].second);
					_bundle._source = _dict.get(_eids[1].first, _eids[1].second);
					_bundle._reportto = _dict.get(_eids[2].first, _eids[2].second);
					_bundle._custodian = _dict.get(_eids[3].first, _eids[3].second);
					break;
				}

				default:
					break;
			}
		}

		void BundleFactory::beginBlock(char type)
		{
			switch (type)
			{
			case 1:	// payload block
				// is adm flag set?
				if (_bundle._procflags & dtn::data::Bundle::APPDATA_IS_ADMRECORD)
				{
					// we expecting a small block, so use memory based blocks
					_block = new dtn::data::PayloadBlock( _blobmanager.create(dtn::blob::BLOBManager::BLOB_MEMORY) );
				}
				else
				{
					_block = new dtn::data::PayloadBlock( _blobmanager.create() );
				}
				break;

			default: // unknown block
				_block = new dtn::data::Block(type);
				break;
			}
		}

		void BundleFactory::endBlock()
		{
			// is adm flag set?
			if (_bundle._procflags & dtn::data::Bundle::APPDATA_IS_ADMRECORD)
			{
				char type;
				_block->getBLOBReference().read(&type, 0, 1);

				dtn::data::Block *block = NULL;

				switch (type >> 4)
				{
				case 1:
					block = new dtn::data::StatusReportBlock(_block);
					break;

				case 2:
					block = new dtn::data::CustodySignalBlock(_block);
					break;
				default:
					// error, block isn't correct
					delete _block;
					_block = NULL;
					return;
				}

				delete _block;
				_block = block;
			}

			_block->read();
			_bundle.addBlock(_block);
			_block = NULL;
		}

		void BundleFactory::beginBlob()
		{
		}

		void BundleFactory::dataBlob(char *data, size_t length)
		{
			_block->getBLOBReference().append(data, length);
		}

		void BundleFactory::endBlob(size_t size)
		{
		}

		dtn::data::Bundle BundleFactory::getBundle()
		{
			return _bundle;
		}
	}
}
