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

			// buffer for all read line calls
			std::string buffer;

			// read all BLOCKs
			while (!_stream.eof() && !lastblock)
			{
				char block_type;

				// read the block type (first line)
				getline(_stream, buffer);

//				// strip off the last char
//				buffer.erase(buffer.size() - 1);

				// abort if the line data is empty
				if (buffer.size() == 0) throw dtn::InvalidDataException("block header is missing");

				// split header value
				std::vector<std::string> values = dtn::utils::Utils::tokenize(":", buffer, 1);

				if (values[0] == "Block")
				{
					std::stringstream ss; ss.str(values[1]);
					ss >> (int&)block_type;
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
						if (obj.get(dtn::data::Bundle::APPDATA_IS_ADMRECORD))
						{
							// create a temporary block
							dtn::data::ExtensionBlock &block = obj.push_back<dtn::data::ExtensionBlock>();

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

							// remove the temporary block
							obj.remove(block);

							switch (admfield >> 4)
							{
								case 1:
								{
									dtn::data::StatusReportBlock &block = obj.push_back<dtn::data::StatusReportBlock>();
									deserializer >> block;
									lastblock = block.get(dtn::data::Block::LAST_BLOCK);
									break;
								}

								case 2:
								{
									dtn::data::CustodySignalBlock &block = obj.push_back<dtn::data::CustodySignalBlock>();
									deserializer >> block;
									lastblock = block.get(dtn::data::Block::LAST_BLOCK);
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
							dtn::data::PayloadBlock &block = obj.push_back<dtn::data::PayloadBlock>();
							(*this) >> block;

							lastblock = block.get(dtn::data::Block::LAST_BLOCK);
						}
						break;
					}

					default:
					{
						// get a extension block factory
						try {
							dtn::data::ExtensionBlock::Factory &f = dtn::data::ExtensionBlock::Factory::get(block_type);

							dtn::data::Block &block = obj.push_back(f);
							(*this) >> block;
							lastblock = block.get(dtn::data::Block::LAST_BLOCK);

							if (block.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
							{
								IBRCOMMON_LOGGER_DEBUG(5) << "unprocessable block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;

								// remove the block
								obj.remove(block);
							}
						}
						catch (const ibrcommon::Exception &ex)
						{
							dtn::data::ExtensionBlock &block = obj.push_back<dtn::data::ExtensionBlock>();
							(*this) >> block;
							lastblock = block.get(dtn::data::Block::LAST_BLOCK);

							if (block.get(dtn::data::Block::DISCARD_IF_NOT_PROCESSED))
							{
								IBRCOMMON_LOGGER_DEBUG(5) << "unprocessable block in bundle " << obj.toString() << " has been removed" << IBRCOMMON_LOGGER_ENDL;

								// remove the block
								obj.remove(block);
							}
						}
						break;
					}
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
			ibrcommon::Base64Reader base64_decoder(_stream, blocksize);
			obj.deserialize(base64_decoder, blocksize);

			// read the final empty line
			std::string buffer;
			getline(_stream, buffer);

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
	}
}
