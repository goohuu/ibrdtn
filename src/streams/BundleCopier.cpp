/*
 * BundleFactory.cpp
 *
 *  Created on: 03.06.2009
 *      Author: morgenro
 */

#include "ibrdtn/streams/BundleCopier.h"
#include "ibrcommon/data/BLOB.h"
#include "ibrdtn/data/Block.h"
#include "ibrdtn/data/PayloadBlock.h"
#include "ibrdtn/data/StatusReportBlock.h"
#include "ibrdtn/data/CustodySignalBlock.h"
#include "ibrdtn/data/SDNV.h"
#include "ibrdtn/data/Exceptions.h"

#include <stdlib.h>

namespace dtn
{
	namespace streams
	{
		BundleCopier::BundleCopier(dtn::data::Bundle &b)
		 : _bundle(b), _block(NULL), _blockref(-1), _state(IDLE)
		{
		}

		BundleCopier::~BundleCopier()
		{
			if (_block != NULL) delete _block;
		}

		void BundleCopier::beginBundle(char version)
		{
			// clear all blocks
			_bundle.clearBlocks();
			_state = PRIMARY_BLOCK;
		}

		/**
		 * This method checks the received data. If the bundle is not received
		 * completely then transform it into a fragment.
		 *
		 * The State of this class tells what to do:
		 *
		 *   PRIMARY_BLOCK:
		 *     The primary block of a bundle is necessary to build a fragment.
		 *     This bundle can not be used for anything.
		 *
		 *   PAYLOAD_BLOCK_HEADER:
		 *     The header of a payload block has been received, but no data.
		 *     Mark the bundle as fragment and discard the payload block.
		 *
		 *   PAYLOAD_BLOCK_DATA:
		 *     Some payload has been received. Create a fragment of this
		 *     bundle.
		 *
		 *   EXTENSION_BLOCK:
		 *     A header of a extension block has been received. It is not allowed
		 *     to split this type of block.
		 *
		 *   ADM_BLOCK:
		 *     A header of a adm block has been received. It is not allowed
		 *     to split this type of block.
		 *
		 *   FINISHED:
		 *     The whole bundle has been received successfully.
		 */
		void BundleCopier::endBundle()
		{
			switch (_state)
			{
			case PRIMARY_BLOCK:
				throw dtn::exceptions::IOException("Not enough data has been received.");

			case PAYLOAD_BLOCK_DATA:
				// create a fragment
				if (!(_bundle._procflags & dtn::data::Bundle::FRAGMENT))
					_bundle._procflags += dtn::data::Bundle::FRAGMENT;

				// set the application length
				_bundle._appdatalength = _last_blocksize;

				// close the blob and add the block to the bundle
				endBlob(0);
				endBlock();
				break;

			case PAYLOAD_BLOCK_HEADER:
			case EXTENSION_BLOCK:
			case ADM_BLOCK:
				// discard the current block
				delete _block;
				_block = NULL;

			case FINISHED:
				// nothing to do
				break;
			};
		}

		void BundleCopier::beginAttribute(ATTRIBUTES attr)
		{
			_next = attr;
		}

		void BundleCopier::endAttribute(char value)
		{

		}

		void BundleCopier::endAttribute(size_t value)
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

				case BLOCK_LENGTH:
					_last_blocksize = value;
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

		void BundleCopier::endAttribute(char *data, size_t size)
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

		void BundleCopier::beginBlock(char type)
		{
			switch (type)
			{
			case 1:	// payload block
				// is adm flag set?
				if (_bundle._procflags & dtn::data::Bundle::APPDATA_IS_ADMRECORD)
				{
					_state = ADM_BLOCK;
					// we expecting a small block, so use memory based blocks
					_block = new dtn::data::PayloadBlock( ibrcommon::StringBLOB::create() );
				}
				else
				{
					_state = PAYLOAD_BLOCK_HEADER;
					_block = new dtn::data::PayloadBlock( ibrcommon::TmpFileBLOB::create() );
				}
				break;

			default: // unknown block
				_state = EXTENSION_BLOCK;
				_block = new dtn::data::Block(type);
				break;
			}
		}

		void BundleCopier::endBlock()
		{
			// is adm flag set?
			if (_bundle._procflags & dtn::data::Bundle::APPDATA_IS_ADMRECORD)
			{
				char type;
				(*_block->getBLOB()) >> type;

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
			_state = IDLE;
		}

		void BundleCopier::beginBlob()
		{
			_state = PAYLOAD_BLOCK_DATA;
		}

		void BundleCopier::dataBlob(char *data, size_t length)
		{
			ibrcommon::BLOB::Reference ref = _block->getBLOB();
			ibrcommon::MutexLock l(ref);
			(*ref).seekp(0, ios_base::end);
			(*ref).write(data, length);
		}

		void BundleCopier::endBlob(size_t size)
		{
			_state = IDLE;
		}
	}
}
