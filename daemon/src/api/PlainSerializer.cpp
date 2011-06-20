/*
 * PlainSerializer.cpp
 *
 *  Created on: 16.06.2011
 *      Author: morgenro
 */

#include "config.h"
#include "api/PlainSerializer.h"
#include <ibrdtn/utils/Utils.h>
#include <ibrcommon/refcnt_ptr.h>
#include <ibrcommon/data/Base64Stream.h>
#include <list>

namespace dtn
{
	namespace api
	{
		PlainSerializer::PlainSerializer(std::ostream &stream)
		 : _stream(stream)
		{
		}

		PlainSerializer::~PlainSerializer()
		{
		}

		dtn::data::Serializer& PlainSerializer::operator<<(const dtn::data::Bundle &obj)
		{
			// serialize the primary block
			(*this) << (dtn::data::PrimaryBlock&)obj;

			// serialize all secondary blocks
			const std::list<const dtn::data::Block*> list = obj.getBlocks();

			// block count
			_stream << "Blocks: " << list.size() << std::endl;

			for (std::list<const dtn::data::Block*>::const_iterator iter = list.begin(); iter != list.end(); iter++)
			{
				const dtn::data::Block &b = (*(*iter));
				_stream << std::endl;
				(*this) << b;
			}

			_stream << std::endl;

			return (*this);
		}

		dtn::data::Serializer& PlainSerializer::operator<<(const dtn::data::PrimaryBlock &obj)
		{
			_stream << "Processing flags: " << obj._procflags << std::endl;
			_stream << "Timestamp: " << obj._timestamp << std::endl;
			_stream << "Sequencenumber: " << obj._sequencenumber << std::endl;
			_stream << "Source: " << obj._source.getString() << std::endl;
			_stream << "Destination: " << obj._destination.getString() << std::endl;
			_stream << "Reportto: " << obj._reportto.getString() << std::endl;
			_stream << "Custodian: " << obj._custodian.getString() << std::endl;
			_stream << "Lifetime: " << obj._lifetime << std::endl;

			if (obj._procflags & dtn::data::PrimaryBlock::FRAGMENT)
			{
				_stream << "Fragment offset: " << obj._fragmentoffset << std::endl;
				_stream << "Application data length: " << obj._appdatalength << std::endl;
			}

			return (*this);
		}

		dtn::data::Serializer& PlainSerializer::operator<<(const dtn::data::Block &obj)
		{
			_stream << "Block: " << (int)obj.getType() << std::endl;

			std::stringstream flags;

			if (obj.get(dtn::data::Block::REPLICATE_IN_EVERY_FRAGMENT))
			{
				flags << " REPLICATE_IN_EVERY_FRAGMENT";
			}

			if (obj.get(dtn::data::Block::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED))
			{
				flags << " TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED";
			}

			if (obj.get(dtn::data::Block::DELETE_BUNDLE_IF_NOT_PROCESSED))
			{
				flags << " DELETE_BUNDLE_IF_NOT_PROCESSED";
			}

			if (obj.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
			{
				flags << " DISCARD_IF_NOT_PROCESSED";
			}

			if (obj.get(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED))
			{
				flags << " FORWARDED_WITHOUT_PROCESSED";
			}

			if (flags.str().length() > 0)
			{
				_stream << "Flags:" << flags.str() << std::endl;
			}

			if (obj.get(dtn::data::Block::BLOCK_CONTAINS_EIDS))
			{
				std::list<dtn::data::EID> eid_list = obj.getEIDList();

				for (std::list<dtn::data::EID>::const_iterator iter = eid_list.begin(); iter != eid_list.end(); iter++)
				{
					_stream << "EID: " << (*iter).getString() << std::endl;
				}
			}

			try {
				_stream << std::endl;

				// put data here
				ibrcommon::Base64Stream b64(_stream, false, 80);
				obj.serialize(b64);
				b64 << std::flush;
			} catch (const std::exception &ex) {
				std::cerr << ex.what() << std::endl;
			}

			_stream << std::endl;

			return (*this);
		}

		size_t PlainSerializer::getLength(const dtn::data::Bundle &obj)
		{
			return 0;
		}

		size_t PlainSerializer::getLength(const dtn::data::PrimaryBlock &obj) const
		{
			return 0;
		}

		size_t PlainSerializer::getLength(const dtn::data::Block &obj) const
		{
			return 0;
		}

		PlainDeserializer::PlainDeserializer(std::istream &stream)
		 : _stream(stream)
		{
		}

		PlainDeserializer::~PlainDeserializer()
		{
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(dtn::data::Bundle &obj)
		{
			// clear all blocks
			obj.clearBlocks();

			// read the primary block
			(*this) >> (dtn::data::PrimaryBlock&)obj;

			// read until the last block
			bool lastblock = false;

//			// read all BLOCKs
//			while (!_stream.eof() && !lastblock)
//			{
//				char block_type;
//
//				// BLOCK_TYPE
//				block_type = _stream.peek();
//
//				switch (block_type)
//				{
//					case 0:
//					{
//						throw dtn::InvalidDataException("block type is zero");
//						break;
//					}
//
//					case dtn::data::PayloadBlock::BLOCK_TYPE:
//					{
//						if (obj.get(dtn::data::Bundle::APPDATA_IS_ADMRECORD))
//						{
//							// create a temporary block
//							dtn::data::ExtensionBlock &block = obj.push_back<dtn::data::ExtensionBlock>();
//
//							// read the block data
//							(*this) >> block;
//
//							// access the payload to get the first byte
//							char admfield;
//							ibrcommon::BLOB::Reference ref = block.getBLOB();
//							ref.iostream()->get(admfield);
//
//							// write the block into a temporary stream
//							stringstream ss;
//							dtn::data::DefaultSerializer serializer(ss, _dictionary);
//							dtn::data::DefaultDeserializer deserializer(ss, _dictionary);
//
//							serializer << block;
//
//							// remove the temporary block
//							obj.remove(block);
//
//							switch (admfield >> 4)
//							{
//								case 1:
//								{
//									dtn::data::StatusReportBlock &block = obj.push_back<dtn::data::StatusReportBlock>();
//									deserializer >> block;
//									lastblock = block.get(Block::LAST_BLOCK);
//									break;
//								}
//
//								case 2:
//								{
//									dtn::data::CustodySignalBlock &block = obj.push_back<dtn::data::CustodySignalBlock>();
//									deserializer >> block;
//									lastblock = block.get(Block::LAST_BLOCK);
//									break;
//								}
//
//								default:
//								{
//									// drop unknown administrative block
//									break;
//								}
//							}
//
//						}
//						else
//						{
//							dtn::data::PayloadBlock &block = obj.push_back<dtn::data::PayloadBlock>();
//							(*this) >> block;
//
//							lastblock = block.get(Block::LAST_BLOCK);
//						}
//						break;
//					}
//
//					default:
//					{
//						// get a extension block factory
//						try {
//							ExtensionBlock::Factory &f = dtn::data::ExtensionBlock::Factory::get(block_type);
//
//							dtn::data::Block &block = obj.push_back(f);
//							(*this) >> block;
//							lastblock = block.get(Block::LAST_BLOCK);
//
//							if (block.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
//							{
//								IBRCOMMON_LOGGER_DEBUG(5) << "unprocessable block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;
//
//								// remove the block
//								obj.remove(block);
//							}
//						}
//						catch (const ibrcommon::Exception &ex)
//						{
//							dtn::data::ExtensionBlock &block = obj.push_back<dtn::data::ExtensionBlock>();
//							(*this) >> block;
//							lastblock = block.get(Block::LAST_BLOCK);
//
//							if (block.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
//							{
//								IBRCOMMON_LOGGER_DEBUG(5) << "unprocessable block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;
//
//								// remove the block
//								obj.remove(block);
//							}
//						}
//						break;
//					}
//				}
//			}

			return (*this);
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(dtn::data::PrimaryBlock &obj)
		{
			std::string data;

			// read until the first empty line appears
			while (_stream.good())
			{
				getline(_stream, data);

				// abort after the first empty line
				if (data[0] == '\n') break;

				// split header value
				std::vector<std::string> values = dtn::utils::Utils::tokenize(":", data, 1);

				// assign header value
				if (values[0] == "Source")
				{
					obj._source = values[1];
				}
			}

			return (*this);
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(dtn::data::Block &obj)
		{
			// read until the first empty line appears

			// then read the payload

			return (*this);
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(std::ostream &stream)
		{
			ibrcommon::Base64Stream b64(stream, true);
			std::string data;

			while (b64.good())
			{
				getline(_stream, data);

				// abort after the first empty line
				if (data[0] == '\n') break;

				// put the line into the stream decoder
				b64 << data;
			}

			b64 << std::flush;

			return (*this);
		}
	}
}
