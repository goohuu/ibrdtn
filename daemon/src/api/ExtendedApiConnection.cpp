/*
 * ExtendedApiConnection.cpp
 *
 *  Created on: 15.06.2011
 *      Author: morgenro
 */

#include "config.h"
#include "Configuration.h"
#include "api/ExtendedApiConnection.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include <ibrcommon/Logger.h>
#include <ibrdtn/api/PlainSerializer.h>
#include <ibrdtn/data/AgeBlock.h>
#include <ibrdtn/utils/Utils.h>
#include "core/BundleCore.h"

#ifdef WITH_COMPRESSION
#include <ibrdtn/data/CompressedPayloadBlock.h>
#endif

#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#endif

namespace dtn
{
	namespace api
	{
		ExtendedApiConnection::ExtendedApiConnection(ExtendedApiServer &server, Registration &registration, ibrcommon::tcpstream *conn)
		 : _sender(*this), _server(server), _registration(registration), _stream(conn), _endpoint(dtn::core::BundleCore::local)
		{
			_stream->exceptions(std::ios::badbit | std::ios::eofbit);

			if ( dtn::daemon::Configuration::getInstance().getNetwork().getTCPOptionNoDelay() )
			{
				_stream->enableNoDelay();
			}
		}

		ExtendedApiConnection::~ExtendedApiConnection()
		{
			_sender.join();
			delete _stream;
		}

		Registration& ExtendedApiConnection::getRegistration()
		{
			return _registration;
		}

		void ExtendedApiConnection::setup()
		{
			_sender.start();
		}

		bool ExtendedApiConnection::__cancellation()
		{
			// close the stream
			try {
				(*_stream).close();
			} catch (const ibrcommon::ConnectionClosedException&) { };

			return true;
		}

		void ExtendedApiConnection::finally()
		{
			IBRCOMMON_LOGGER_DEBUG(60) << "ExtendedApiConnection down" << IBRCOMMON_LOGGER_ENDL;

			// remove the client from the list in ApiServer
			_server.connectionDown(this);

			_registration.getQueue().abort();
			_server.freeRegistration(_registration);

			// close the stream
			try {
				(*_stream).close();
			} catch (const ibrcommon::ConnectionClosedException&) { };

//			try {
//				// shutdown the sender thread
//				_sender.stop();
//			} catch (const std::exception&) { };
		}

		void ExtendedApiConnection::run()
		{
			// signal the active connection to the server
			_server.connectionUp(this);

			std::string buffer;

			while (_stream->good())
			{
				getline(*_stream, buffer);

				std::vector<std::string> cmd = dtn::utils::Utils::tokenize(" ", buffer);
				if (cmd.size() == 0) continue;

				try {
					if (cmd[0] == "set")
					{
						if (cmd.size() < 2) throw ibrcommon::Exception("not enough parameters");

						if (cmd[1] == "endpoint")
						{
							if (cmd.size() < 3) throw ibrcommon::Exception("not enough parameters");

							ibrcommon::MutexLock l(_write_lock);
							_endpoint = dtn::core::BundleCore::local + "/" + cmd[2];

							// error checking
							if (_endpoint == dtn::data::EID())
							{
								(*_stream) << API_STATUS_NOT_ACCEPTABLE << " INVALID ENDPOINT" << std::endl;
								_endpoint = dtn::core::BundleCore::local;
							}
							else
							{
								_registration.subscribe(_endpoint);
								(*_stream) << API_STATUS_ACCEPTED << " OK" << std::endl;
							}
						}
						else
						{
							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
						}
					}
					else if (cmd[0] == "registration")
					{
						if (cmd.size() < 2) throw ibrcommon::Exception("not enough parameters");

						if (cmd[1] == "add")
						{
							if (cmd.size() < 3) throw ibrcommon::Exception("not enough parameters");

							ibrcommon::MutexLock l(_write_lock);
							dtn::data::EID endpoint(cmd[2]);

							// error checking
							if (endpoint == dtn::data::EID())
							{
								(*_stream) << API_STATUS_NOT_ACCEPTABLE << " INVALID EID" << std::endl;
							}
							else
							{
								_registration.subscribe(endpoint);
								(*_stream) << API_STATUS_ACCEPTED << " OK" << std::endl;
							}
						}
						else if (cmd[1] == "del")
						{
							if (cmd.size() < 3) throw ibrcommon::Exception("not enough parameters");

							ibrcommon::MutexLock l(_write_lock);
							dtn::data::EID endpoint(cmd[2]);

							// error checking
							if (endpoint == dtn::data::EID())
							{
								(*_stream) << API_STATUS_NOT_ACCEPTABLE << " INVALID EID" << std::endl;
							}
							else
							{
								_registration.unsubscribe(endpoint);
								(*_stream) << API_STATUS_ACCEPTED << " OK" << std::endl;
							}
						}
						else if (cmd[1] == "list")
						{
							ibrcommon::MutexLock l(_write_lock);
							const std::set<dtn::data::EID> &list = _registration.getSubscriptions();

							(*_stream) << API_STATUS_OK << " REGISTRATION LIST" << std::endl;
							for (std::set<dtn::data::EID>::const_iterator iter = list.begin(); iter != list.end(); iter++)
							{
								(*_stream) << (*iter).getString() << std::endl;
							}
							(*_stream) << std::endl;
						}
						else
						{
							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
						}
					}
					else if (cmd[0] == "neighbor")
					{
						if (cmd.size() < 2) throw ibrcommon::Exception("not enough parameters");

						if (cmd[1] == "list")
						{
							ibrcommon::MutexLock l(_write_lock);
							const std::set<dtn::core::Node> nlist = dtn::core::BundleCore::getInstance().getNeighbors();

							(*_stream) << API_STATUS_OK << " NEIGHBOR LIST" << std::endl;
							for (std::set<dtn::core::Node>::const_iterator iter = nlist.begin(); iter != nlist.end(); iter++)
							{
								(*_stream) << (*iter).getEID().getString() << std::endl;
							}
							(*_stream) << std::endl;
						}
						else
						{
							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
						}
					}
					else if (cmd[0] == "bundle")
					{
						if (cmd.size() < 2) throw ibrcommon::Exception("not enough parameters");

						if (cmd[1] == "get")
						{
							// transfer bundle data
							ibrcommon::MutexLock l(_write_lock);

							if (cmd.size() == 2)
							{
								(*_stream) << API_STATUS_OK << " BUNDLE GET "; sayBundleID(*_stream, _bundle_reg); (*_stream) << std::endl;
								PlainSerializer(*_stream) << _bundle_reg;
							}
							else if (cmd[2] == "binary")
							{
								(*_stream) << API_STATUS_OK << " BUNDLE GET BINARY "; sayBundleID(*_stream, _bundle_reg); (*_stream) << std::endl;
								dtn::data::DefaultSerializer(*_stream) << _bundle_reg; (*_stream) << std::flush;
							}
							else if (cmd[2] == "plain")
							{
								(*_stream) << API_STATUS_OK << " BUNDLE GET PLAIN "; sayBundleID(*_stream, _bundle_reg); (*_stream) << std::endl;
								PlainSerializer(*_stream) << _bundle_reg;
							}
							else if (cmd[2] == "xml")
							{
								(*_stream) << API_STATUS_NOT_IMPLEMENTED << " FORMAT NOT IMPLEMENTED" << std::endl;
							}
							else
							{
								(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN FORMAT" << std::endl;
							}
						}
						else if (cmd[1] == "put")
						{
							// lock the stream during reception of bundle data
							ibrcommon::MutexLock l(_write_lock);

							if (cmd.size() < 2)
							{
								(*_stream) << API_STATUS_BAD_REQUEST << " PLEASE DEFINE THE FORMAT" << std::endl;
							}
							else if (cmd[2] == "plain")
							{
								(*_stream) << API_STATUS_CONTINUE << " PUT BUNDLE PLAIN" << std::endl;

								try {
									PlainDeserializer(*_stream) >> _bundle_reg;
									(*_stream) << API_STATUS_OK << " BUNDLE IN REGISTER" << std::endl;
								} catch (const std::exception&) {
									(*_stream) << API_STATUS_NOT_ACCEPTABLE << " PUT FAILED" << std::endl;

								}
							}
							else if (cmd[2] == "binary")
							{
								(*_stream) << API_STATUS_CONTINUE << " PUT BUNDLE BINARY" << std::endl;

								try {
									dtn::data::DefaultDeserializer(*_stream) >> _bundle_reg;
									(*_stream) << API_STATUS_OK << " BUNDLE IN REGISTER" << std::endl;
								} catch (const std::exception&) {
									(*_stream) << API_STATUS_NOT_ACCEPTABLE << " PUT FAILED" << std::endl;
								}
							}
							else
							{
								(*_stream) << API_STATUS_BAD_REQUEST << " PLEASE DEFINE THE FORMAT" << std::endl;
							}
						}
						else if (cmd[1] == "load")
						{
							if (cmd.size() < 3) throw ibrcommon::Exception("not enough parameters");

							dtn::data::BundleID id;

							if (cmd[2] == "queue")
							{
								id = _bundle_queue.getnpop();
							}
							else
							{
								// construct bundle id
								id = readBundleID(cmd, 2);
							}

							// load the bundle
							try {
								_bundle_reg = dtn::core::BundleCore::getInstance().getStorage().get(id);

								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_OK << " BUNDLE LOADED "; sayBundleID(*_stream, id); (*_stream) << std::endl;
							} catch (const ibrcommon::Exception&) {
								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_NOT_FOUND << " BUNDLE NOT FOUND" << std::endl;
							}
						}
						else if (cmd[1] == "clear")
						{
							_bundle_reg = dtn::data::Bundle();

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " BUNDLE CLEARED" << std::endl;
						}
						else if (cmd[1] == "free")
						{
							try {
								dtn::core::BundleCore::getInstance().getStorage().remove(_bundle_reg);
								_bundle_reg = dtn::data::Bundle();
								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_OK << " BUNDLE FREE SUCCESSFUL" << std::endl;
							} catch (const ibrcommon::Exception&) {
								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_NOT_FOUND << " BUNDLE NOT FOUND" << std::endl;
							}
						}
						else if (cmd[1] == "delivered")
						{
							if (cmd.size() < 3) throw ibrcommon::Exception("not enough parameters");

							try {
								// construct bundle id
								dtn::data::BundleID id = readBundleID(cmd, 2);
								dtn::data::MetaBundle meta = dtn::core::BundleCore::getInstance().getStorage().get(id);

								// raise bundle event
								dtn::core::BundleEvent::raise(meta, BUNDLE_DELIVERED);

								// delete it if it was a singleton
								if (meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON))
								{
									dtn::core::BundleCore::getInstance().getStorage().remove(id);
								}

								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_OK << " BUNDLE DELIVERED ACCEPTED" << std::endl;
							} catch (const ibrcommon::Exception&) {
								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_NOT_FOUND << " BUNDLE NOT FOUND" << std::endl;
							}
						}
						else if (cmd[1] == "store")
						{
							// store the bundle in the storage
							try {
								dtn::core::BundleCore::getInstance().getStorage().store(_bundle_reg);
								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_OK << " BUNDLE STORE SUCCESSFUL" << std::endl;
							} catch (const ibrcommon::Exception&) {
								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_INTERNAL_ERROR << " BUNDLE STORE FAILED" << std::endl;
							}
						}
						else if (cmd[1] == "send")
						{
							processIncomingBundle(_bundle_reg);

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " BUNDLE SENT" << std::endl;
						}
						else
						{
							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
						}
					}
					else
					{
						ibrcommon::MutexLock l(_write_lock);
						(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
					}
				} catch (const std::exception&) {
					ibrcommon::MutexLock l(_write_lock);
					(*_stream) << API_STATUS_BAD_REQUEST << " ERROR" << std::endl;
				}
			}
		}

		void ExtendedApiConnection::eventNodeAvailable(const dtn::core::Node &node)
		{
			ibrcommon::MutexLock l(_write_lock);
			(*_stream) << API_STATUS_NOTIFY_NEIGHBOR << " NOTIFY NODE AVAILABLE " << node.getEID().getString() << std::endl;
		}

		void ExtendedApiConnection::eventNodeUnavailable(const dtn::core::Node &node)
		{
			ibrcommon::MutexLock l(_write_lock);
			(*_stream) << API_STATUS_NOTIFY_NEIGHBOR << " NOTIFY NODE UNAVAILABLE " << node.getEID().getString() << std::endl;
		}

		ExtendedApiConnection::Sender::Sender(ExtendedApiConnection &conn)
		 : _conn(conn)
		{
		}

		ExtendedApiConnection::Sender::~Sender()
		{
			ibrcommon::JoinableThread::join();
		}

		bool ExtendedApiConnection::Sender::__cancellation()
		{
			// cancel the main thread in here
			//_conn.getRegistration().getQueue().abort();

			return true;
		}

		void ExtendedApiConnection::Sender::finally()
		{
			//_conn._server.freeRegistration(_conn.getRegistration());
		}

		void ExtendedApiConnection::Sender::run()
		{
			ibrcommon::Queue<dtn::data::BundleID> &queue = _conn.getRegistration().getQueue();

			try {
				while (_conn._stream->good())
				{
					dtn::data::BundleID id = queue.getnpop(true);

					// move the bundle to the local bundle queue
					_conn._bundle_queue.push(id);

					// notify the client about the new bundle
					{
						ibrcommon::MutexLock l(_conn._write_lock);
						(*_conn._stream) << API_STATUS_NOTIFY_BUNDLE << " NOTIFY BUNDLE ";
						sayBundleID(*_conn._stream, id);
						(*_conn._stream) << std::endl;
					}

					// idle a little bit
					yield();
				}
			} catch (const ibrcommon::QueueUnblockedException &ex) {
				IBRCOMMON_LOGGER_DEBUG(40) << "ClientHandler::Sender::run(): aborted" << IBRCOMMON_LOGGER_ENDL;
				return;
			} catch (const ibrcommon::IOException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: IOException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const dtn::InvalidDataException &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "API: InvalidDataException says " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			} catch (const std::exception &ex) {
				IBRCOMMON_LOGGER_DEBUG(10) << "unexpected API error! " << ex.what() << IBRCOMMON_LOGGER_ENDL;
			}

//			try {
//				_conn.stop();
//			} catch (const ibrcommon::ThreadException &ex) {
//				IBRCOMMON_LOGGER_DEBUG(50) << "ClientHandler::Sender::run(): ThreadException (" << ex.what() << ") on termination" << IBRCOMMON_LOGGER_ENDL;
//			}
		}

		void ExtendedApiConnection::processIncomingBundle(dtn::data::Bundle &bundle)
		{
			// create a new sequence number
			bundle.relabel();

			// check address fields for "api:me", this has to be replaced
			static const dtn::data::EID clienteid("api:me");

			// set the source address to the sending EID
			bundle._source = _endpoint;

			if (bundle._destination == clienteid) bundle._destination = _endpoint;
			if (bundle._reportto == clienteid) bundle._reportto = _endpoint;
			if (bundle._custodian == clienteid) bundle._custodian = _endpoint;

			// if the timestamp is not set, add a ageblock
			if (bundle._timestamp == 0)
			{
				// check for ageblock
				try {
					bundle.getBlock<dtn::data::AgeBlock>();
				} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) {
					// add a new ageblock
					bundle.push_front<dtn::data::AgeBlock>();
				}
			}

#ifdef WITH_COMPRESSION
			// if the compression bit is set, then compress the bundle
			if (bundle.get(dtn::data::PrimaryBlock::IBRDTN_REQUEST_COMPRESSION))
			{
				try {
					dtn::data::CompressedPayloadBlock::compress(bundle, dtn::data::CompressedPayloadBlock::COMPRESSION_ZLIB);
				} catch (const ibrcommon::Exception &ex) {
					IBRCOMMON_LOGGER(warning) << "compression of bundle failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				};
			}
#endif

#ifdef WITH_BUNDLE_SECURITY
			// if the encrypt bit is set, then try to encrypt the bundle
			if (bundle.get(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT))
			{
				try {
					dtn::security::SecurityManager::getInstance().encrypt(bundle);

					bundle.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT, false);
				} catch (const dtn::security::SecurityManager::KeyMissingException&) {
					// sign requested, but no key is available
					IBRCOMMON_LOGGER(warning) << "No key available for encrypt process." << IBRCOMMON_LOGGER_ENDL;
				} catch (const dtn::security::SecurityManager::EncryptException&) {
					IBRCOMMON_LOGGER(warning) << "Encryption of bundle failed." << IBRCOMMON_LOGGER_ENDL;
				}
			}

			// if the sign bit is set, then try to sign the bundle
			if (bundle.get(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN))
			{
				try {
					dtn::security::SecurityManager::getInstance().sign(bundle);

					bundle.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN, false);
				} catch (const dtn::security::SecurityManager::KeyMissingException&) {
					// sign requested, but no key is available
					IBRCOMMON_LOGGER(warning) << "No key available for sign process." << IBRCOMMON_LOGGER_ENDL;
				}
			}
#endif

			// raise default bundle received event
			dtn::net::BundleReceivedEvent::raise(dtn::core::BundleCore::local + getRegistration().getHandle(), bundle, true);
		}

		void ExtendedApiConnection::sayBundleID(ostream &stream, const dtn::data::BundleID &id)
		{
			stream << id.timestamp << " " << id.sequencenumber << " ";

			if (id.fragment)
			{
				stream << id.offset << " ";
			}

			stream << id.source.getString();
		}

		dtn::data::BundleID ExtendedApiConnection::readBundleID(const std::vector<std::string> &data, const size_t start)
		{
			// load bundle id
			std::stringstream ss;
			size_t timestamp = 0;
			size_t sequencenumber = 0;
			bool fragment = false;
			size_t offset = 0;

			// read timestamp
			 ss.clear(); ss.str(data[start]); ss >> timestamp;

			// read sequence number
			 ss.clear(); ss.str(data[start+1]); ss >> sequencenumber;

			// read fragment offset
			if ((data.size() - start) > 3)
			{
				fragment = true;

				// read sequence number
				 ss.clear(); ss.str(data[start+2]); ss >> offset;
			}

			// read EID
			 ss.clear(); dtn::data::EID eid(data[data.size() - 1]);

			// construct bundle id
			return dtn::data::BundleID(eid, timestamp, sequencenumber, fragment, offset);
		}
	}
}
