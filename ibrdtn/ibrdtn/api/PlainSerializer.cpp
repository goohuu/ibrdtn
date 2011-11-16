/*
 * PlainSerializer.cpp
 *
 *  Created on: 16.06.2011
 *      Author: morgenro
 */

#include "config.h"
#include "ibrdtn/api/PlainSerializer.h"
#include "ibrdtn/utils/Utils.h"
#include <ibrcommon/refcnt_ptr.h>
#include <ibrcommon/data/Base64Stream.h>
#include <ibrcommon/data/Base64Reader.h>
#include <ibrcommon/Logger.h>
#include <list>

namespace dtn
{
	namespace api
	{
		PlainSerializer::PlainSerializer(std::ostream &stream, bool skip_payload)
		 : _stream(stream), _skip_payload(skip_payload)
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

			if (obj.get(dtn::data::Block::LAST_BLOCK))
			{
				flags << " LAST_BLOCK";
			}

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

			_stream << "Length: " << obj.getLength() << std::endl;


			if(!_skip_payload){
				try {

					_stream << std::endl;

					// put data here
					ibrcommon::Base64Stream b64(_stream, false, 80);
					size_t slength = 0;
					obj.serialize(b64, slength);
					b64 << std::flush;
				} catch (const std::exception &ex) {
					std::cerr << ex.what() << std::endl;
				}

				_stream << std::endl;
			}

			return (*this);
		}

		dtn::data::Serializer &PlainSerializer::serialize(ibrcommon::BLOB::iostream &obj, size_t limit){
			size_t len = obj.size();
			if(limit < len && limit > 0)
			{
				len = limit;
			}

			_stream << "Length: " << len << std::endl;
			_stream << std::endl;

			ibrcommon::Base64Stream b64(_stream, false, 80);
			ibrcommon::BLOB::copy(b64, *obj, len);
			b64 << std::flush;

			_stream << std::endl;

			return *this;
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

			// read all BLOCKs
			while (!_stream.eof() && !lastblock)
			{
				try
				{
					dtn::data::Block& block = readBlock(BlockInserter(obj, BlockInserter::END), obj.get(dtn::data::Bundle::APPDATA_IS_ADMRECORD));
					lastblock = block.get(dtn::data::Block::LAST_BLOCK);
				}
				catch (const UnknownBlockException &ex)
				{
					IBRCOMMON_LOGGER_DEBUG(5) << "unknown administrative block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;
				}
				catch (const BlockNotProcessableException &ex)
				{
					IBRCOMMON_LOGGER_DEBUG(5) << "unprocessable block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;
				}
			}

			return (*this);
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(dtn::data::PrimaryBlock &obj)
		{
			std::string data;

			// read until the first empty line appears
			while (_stream.good())
			{
				std::stringstream ss;
				getline(_stream, data);

				std::string::reverse_iterator iter = data.rbegin();
				if ( (*iter) == '\r' ) data = data.substr(0, data.length() - 1);

//				// strip off the last char
//				data.erase(data.size() - 1);

				// abort after the first empty line
				if (data.size() == 0) break;

				// split header value
				std::vector<std::string> values = dtn::utils::Utils::tokenize(":", data, 1);

				// if there are not enough parameter abort with an error
				if (values.size() < 1) throw ibrcommon::Exception("parsing error");

				// assign header value
				if (values[0] == "Processing flags")
				{
					ss.clear(); ss.str(values[1]);
					ss >> obj._procflags;
				}
				else if (values[0] == "Timestamp")
				{
					ss.clear(); ss.str(values[1]);
					ss >> obj._timestamp;
				}
				else if (values[0] == "Sequencenumber")
				{
					ss.clear(); ss.str(values[1]);
					ss >> obj._sequencenumber;
				}
				else if (values[0] == "Source")
				{
					obj._source = values[1];
				}
				else if (values[0] == "Destination")
				{
					obj._destination = values[1];
				}
				else if (values[0] == "Reportto")
				{
					obj._reportto = values[1];
				}
				else if (values[0] == "Custodian")
				{
					obj._custodian = values[1];
				}
				else if (values[0] == "Lifetime")
				{
					ss.clear(); ss.str(values[1]);
					ss >> obj._lifetime;
				}
				else if (values[0] == "Fragment offset")
				{
					ss.clear(); ss.str(values[1]);
					ss >> obj._fragmentoffset;
				}
				else if (values[0] == "Application data length")
				{
					ss.clear(); ss.str(values[1]);
					ss >> obj._appdatalength;
				}
			}

			return (*this);
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(dtn::data::Block &obj)
		{
			std::string data;
			size_t blocksize = 0;

			// read until the first empty line appears
			while (_stream.good())
			{
				getline(_stream, data);

				std::string::reverse_iterator iter = data.rbegin();
				if ( (*iter) == '\r' ) data = data.substr(0, data.length() - 1);

//				// strip off the last char
//				data.erase(data.size() - 1);

				// abort after the first empty line
				if (data.size() == 0) break;

				// split header value
				std::vector<std::string> values = dtn::utils::Utils::tokenize(":", data, 1);

				// assign header value
				if (values[0] == "Flags")
				{
					std::vector<std::string> flags = dtn::utils::Utils::tokenize(" ", values[1]);

					for (std::vector<std::string>::const_iterator iter = flags.begin(); iter != flags.end(); iter++)
					{
						const std::string &value = (*iter);
						if (value == "LAST_BLOCK")
						{
							obj.set(dtn::data::Block::LAST_BLOCK, true);
						}
						else if (value == "FORWARDED_WITHOUT_PROCESSED")
						{
							obj.set(dtn::data::Block::FORWARDED_WITHOUT_PROCESSED, true);
						}
						else if (value == "DISCARD_IF_NOT_PROCESSED")
						{
							obj.set(dtn::data::Block::DISCARD_IF_NOT_PROCESSED, true);
						}
						else if (value == "DELETE_BUNDLE_IF_NOT_PROCESSED")
						{
							obj.set(dtn::data::Block::DELETE_BUNDLE_IF_NOT_PROCESSED, true);
						}
						else if (value == "TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED")
						{
							obj.set(dtn::data::Block::TRANSMIT_STATUSREPORT_IF_NOT_PROCESSED, true);
						}
						else if (value == "REPLICATE_IN_EVERY_FRAGMENT")
						{
							obj.set(dtn::data::Block::REPLICATE_IN_EVERY_FRAGMENT, true);
						}
					}
				}
				else if (values[0] == "EID")
				{
					obj.addEID(values[1]);
				}
				else if (values[0] == "Length")
				{
					std::stringstream ss; ss.str(values[1]);
					ss >> blocksize;
				}
			}

			// then read the payload
			IBRCOMMON_LOGGER_DEBUG(20) << "API expecting payload size of " << blocksize << IBRCOMMON_LOGGER_ENDL;
			ibrcommon::Base64Reader base64_decoder(_stream, blocksize);
			obj.deserialize(base64_decoder, blocksize);

			std::string buffer;

			// read the appended newline character
			getline(_stream, buffer);
			std::string::reverse_iterator iter = buffer.rbegin();
			if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);
			if (buffer.size() != 0) throw dtn::InvalidDataException("last line not empty");

			// read the final empty line
			getline(_stream, buffer);
			iter = buffer.rbegin();
			if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);
			if (buffer.size() != 0) throw dtn::InvalidDataException("last line not empty");

			return (*this);
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(ibrcommon::BLOB::iostream &obj)
		{
			std::string data;
			size_t blocksize = 0;

			// read until the first empty line appears
			while (_stream.good())
			{
				getline(_stream, data);

				std::string::reverse_iterator iter = data.rbegin();
				if ( (*iter) == '\r' ) data = data.substr(0, data.length() - 1);

				// abort after the first empty line
				if (data.size() == 0) break;

				// split header value
				std::vector<std::string> values = dtn::utils::Utils::tokenize(":", data, 1);

				// assign header value
				if (values[0] == "Length")
				{
					std::stringstream ss; ss.str(values[1]);
					ss >> blocksize;
				}
			}

			// then read the payload
			ibrcommon::Base64Reader base64_decoder(_stream, blocksize);
			ibrcommon::BLOB::copy(*obj, base64_decoder, blocksize);
			//*obj.iostream() << base64_decoder.rdbuf() << std::flush;

			std::string buffer;

			// read the appended newline character
			getline(_stream, buffer);
			std::string::reverse_iterator iter = buffer.rbegin();
			if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);
			if (buffer.size() != 0) throw dtn::InvalidDataException("last line not empty");

			// read the final empty line
			getline(_stream, buffer);
			iter = buffer.rbegin();
			if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);
			if (buffer.size() != 0) throw dtn::InvalidDataException("last line not empty");

			return (*this);
		}

		dtn::data::Deserializer& PlainDeserializer::operator>>(std::ostream &stream)
		{
			ibrcommon::Base64Stream b64(stream, true);
			std::string data;

			while (b64.good())
			{
				getline(_stream, data);

				std::string::reverse_iterator iter = data.rbegin();
				if ( (*iter) == '\r' ) data = data.substr(0, data.length() - 1);

//				// strip off the last char
//				data.erase(data.size() - 1);

				// abort after the first empty line
				if (data.size() == 0) break;

				// put the line into the stream decoder
				b64 << data;
			}

			b64 << std::flush;

			return (*this);
		}

		dtn::data::Block& PlainDeserializer::readBlock(BlockInserter inserter, bool payload_is_adm)
		{
			std::string buffer;
			int block_type;

			// read the block type (first line)
			getline(_stream, buffer);

			std::string::reverse_iterator iter = buffer.rbegin();
			if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);

			// abort if the line data is empty
			if (buffer.size() == 0) throw dtn::InvalidDataException("block header is missing");

			// split header value
			std::vector<std::string> values = dtn::utils::Utils::tokenize(":", buffer, 1);

			if (values[0] == "Block")
			{
				std::stringstream ss; ss.str(values[1]);
				ss >> block_type;
			}
			else
			{
				throw dtn::InvalidDataException("need block type as first header");
			}

			switch (block_type)
			{
				case 0:
				{
					throw dtn::InvalidDataException("block type is zero");
					break;
				}

				case dtn::data::PayloadBlock::BLOCK_TYPE:
				{
					//if (payload_is_adm)
					if (payload_is_adm)
					{
						// create a temporary block
						dtn::data::ExtensionBlock block;
						block.set(dtn::data::Block::LAST_BLOCK, false);

						// read the block data
						(*this) >> block;

						// access the payload to get the first byte
						char admfield;
						ibrcommon::BLOB::Reference ref = block.getBLOB();
						ref.iostream()->get(admfield);

						// write the block into a temporary stream
						stringstream ss;
						PlainSerializer serializer(ss);
						PlainDeserializer deserializer(ss);

						serializer << block;

						switch (admfield >> 4)
						{
							case 1:
							{
								dtn::data::StatusReportBlock &block = inserter.insert<dtn::data::StatusReportBlock>();
								block.set(dtn::data::Block::LAST_BLOCK, false);
								deserializer >> block;
								//lastblock = block.get(dtn::data::Block::LAST_BLOCK);
								return block;
							}

							case 2:
							{
								dtn::data::CustodySignalBlock &block = inserter.insert<dtn::data::CustodySignalBlock>();
								block.set(dtn::data::Block::LAST_BLOCK, false);
								deserializer >> block;
								//lastblock = block.get(dtn::data::Block::LAST_BLOCK);
								//break;
								return block;
							}

							default:
							{
								// drop unknown administrative block
								throw UnknownBlockException("unknown administrative record");
								break;
							}
						}

					}
					else
					{
						dtn::data::PayloadBlock &block = inserter.insert<dtn::data::PayloadBlock>();
						block.set(dtn::data::Block::LAST_BLOCK, false);
						(*this) >> block;

						return block;
					}
				}

				default:
				{
					// get a extension block factory
					try {
						dtn::data::ExtensionBlock::Factory &f = dtn::data::ExtensionBlock::Factory::get((char) block_type);

						dtn::data::Block &block = inserter.insert(f);
						block.set(dtn::data::Block::LAST_BLOCK, false);
						(*this) >> block;

						if (block.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
						{
							//IBRCOMMON_LOGGER_DEBUG(5) << "unprocessable block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;

							throw BlockNotProcessableException();
						}
						return block;
					}
					catch (const BlockNotProcessableException &ex){
						throw ex;
					}
					catch (const ibrcommon::Exception &ex)
					{
						dtn::data::ExtensionBlock &block = inserter.insert<dtn::data::ExtensionBlock>();
						block.set(dtn::data::Block::LAST_BLOCK, false);
						(*this) >> block;

						if (block.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
						{
							//IBRCOMMON_LOGGER_DEBUG(5) << "unprocessable block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;

							throw BlockNotProcessableException();
						}
						return block;
					}
				}
			}
		}

		PlainDeserializer::BlockInserter::BlockInserter(dtn::data::Bundle &bundle, POSITION alignment, int pos)
			:	_bundle(&bundle), _alignment(alignment), _pos(pos)
		{
		}

		dtn::data::Block &PlainDeserializer::BlockInserter::insert(dtn::data::ExtensionBlock::Factory &f)
		{
			switch (_alignment)
			{
			case FRONT:
				return _bundle->push_front(f);
			case END:
				return _bundle->push_back(f);
			default:
				if(_pos <= 0)
					return _bundle->push_front(f);

				try
				{
					dtn::data::Block &prev_block = _bundle->getBlock(_pos-1);
					return _bundle->insert(f, prev_block);
				}
				catch (const std::exception &ex)
				{
					return _bundle->push_back(f);
				}
			}
		}

		PlainDeserializer::BlockInserter::POSITION PlainDeserializer::BlockInserter::getAlignment() const
		{
			return _alignment;
		}
	}
}
