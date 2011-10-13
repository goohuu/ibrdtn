/*
 * ClientHandler.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "config.h"
#include "Configuration.h"
#include "api/ClientHandler.h"
#include "api/BinaryStreamClient.h"
#include "core/BundleCore.h"
#include <ibrcommon/Logger.h>
#include <ibrdtn/utils/Utils.h>
#include "ExtendedApiHandler.h"
#include <ibrcommon/net/LinkManager.h>

namespace dtn
{
	namespace api
	{
		ProtocolHandler::ProtocolHandler(ClientHandler &client, ibrcommon::tcpstream &stream)
		 : _client(client), _stream(stream)
		{
		}

		ProtocolHandler::~ProtocolHandler()
		{}

		ClientHandler::ClientHandler(ApiServerInterface &srv, Registration &registration, ibrcommon::tcpstream *conn)
		 : _srv(srv), _registration(registration), _stream(conn), _endpoint(dtn::core::BundleCore::local), _handler(NULL)
		{
			if ( dtn::daemon::Configuration::getInstance().getNetwork().getTCPOptionNoDelay() )
			{
				_stream->enableNoDelay();
			}
		}

		ClientHandler::~ClientHandler()
		{
			delete _stream;
		}

		Registration& ClientHandler::getRegistration()
		{
			return _registration;
		}

		ApiServerInterface& ClientHandler::getAPIServer()
		{
			return _srv;
		}

		void ClientHandler::setup()
		{
		}

		void ClientHandler::run()
		{
			// signal the active connection to the server
			_srv.connectionUp(this);

			std::string buffer;

			while (_stream->good())
			{
				if (_handler != NULL)
				{
					_handler->setup();
					_handler->run();
					delete _handler;
					_handler = NULL;
					continue;
				}

				getline(*_stream, buffer);

				// search for '\r\n' and remove the '\r'
				std::string::reverse_iterator iter = buffer.rbegin();
				if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);

				std::vector<std::string> cmd = dtn::utils::Utils::tokenize(" ", buffer);
				if (cmd.size() == 0) continue;

				try {
					if (cmd[0] == "protocol")
					{
						if (cmd[1] == "tcpcl")
						{
							// switch to binary protocol (old style api)
							_handler = new BinaryStreamClient(*this, *_stream);
							continue;
						}
						else if (cmd[1] == "extended")
						{
							// switch to the extended api
							_handler = new ExtendedApiHandler(*this, *_stream);
							continue;
						}
						else
						{
							error(API_STATUS_NOT_ACCEPTABLE, "UNKNOWN PROTOCOL");
						}
					}
					else
					{
						// forward to standard command set
						processCommand(cmd);
					}
				} catch (const std::exception&) {
					error(API_STATUS_BAD_REQUEST, "PROTOCOL ERROR");
				}
			}
		}

		void ClientHandler::error(STATUS_CODES code, const std::string &msg)
		{
			ibrcommon::MutexLock l(_write_lock);
			(*_stream) << code << " " << msg << std::endl;
		}

		bool ClientHandler::__cancellation()
		{
			// close the stream
			(*_stream).close();

			return true;
		}

		void ClientHandler::finally()
		{
			IBRCOMMON_LOGGER_DEBUG(60) << "ApiConnection down" << IBRCOMMON_LOGGER_ENDL;

			// remove the client from the list in ApiServer
			_srv.connectionDown(this);

			_registration.abort();
			_srv.freeRegistration(_registration);

			// close the stream
			try {
				(*_stream).close();
			} catch (const ibrcommon::ConnectionClosedException&) { };
		}

		void ClientHandler::eventNodeAvailable(const dtn::core::Node &node)
		{
		}

		void ClientHandler::eventNodeUnavailable(const dtn::core::Node &node)
		{
		}

		void ClientHandler::processCommand(const std::vector<std::string> &cmd)
		{
			class BundleFilter : public dtn::core::BundleStorage::BundleFilterCallback
			{
			public:
				BundleFilter()
				{};

				virtual ~BundleFilter() {};

				virtual size_t limit() const { return 0; };

				virtual bool shouldAdd(const dtn::data::MetaBundle &meta) const
				{
					return true;
				}
			};

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
				else if (cmd[0] == "interface")
				{
					if (cmd[1] == "address")
					{
						try {
							ibrcommon::DefaultLinkManager &lm = dynamic_cast<ibrcommon::DefaultLinkManager&>(ibrcommon::LinkManager::getInstance());

							ibrcommon::vaddress::Family f = ibrcommon::vaddress::VADDRESS_UNSPEC;

							if (cmd[4] == "ipv4")	f = ibrcommon::vaddress::VADDRESS_INET;
							if (cmd[4] == "ipv6")	f = ibrcommon::vaddress::VADDRESS_INET6;

							ibrcommon::vinterface iface(cmd[3]);
							ibrcommon::vaddress addr(f, cmd[5]);

							if (cmd[2] == "add")
							{
								lm.addressAdded(iface, addr);

								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_OK << " ADDRESS ADDED" << std::endl;

							}
							else if (cmd[2] == "del")
							{
								lm.addressRemoved(iface, addr);

								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_OK << " ADDRESS REMOVED" << std::endl;
							}
							else
							{
								ibrcommon::MutexLock l(_write_lock);
								(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
							}
						} catch (const std::bad_cast&) {
							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_NOT_ALLOWED << " FEATURE NOT AVAILABLE" << std::endl;
						};
					}
					else
					{
						ibrcommon::MutexLock l(_write_lock);
						(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
					}
				}
				else if (cmd[0] == "connection")
				{
					// need to process the connection arguments
					// the arguments look like:
					// <eid> [tcp] [add|del] <ip> <port>
					dtn::core::Node n(cmd[1]);

					/* FIXME cmd len check */
					if (cmd[2] == "tcp")
					{
						if (cmd[3] == "add")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_TCPIP, uri));
							dtn::core::BundleCore::getInstance().addConnection(n);

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " CONNECTION ADDED" << std::endl;
						}
						else if (cmd[3] == "del")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_TCPIP, uri));
							dtn::core::BundleCore::getInstance().removeConnection(n);

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " CONNECTION REMOVED" << std::endl;
						}
					}
					else if (cmd[2] == "udp")
					{
						if (cmd[3] == "add")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_UDPIP, uri));
							dtn::core::BundleCore::getInstance().addConnection(n);

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " CONNECTION ADDED" << std::endl;
						}
						else if (cmd[3] == "del")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_UDPIP, uri));
							dtn::core::BundleCore::getInstance().removeConnection(n);

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " CONNECTION REMOVED" << std::endl;
						}
					}
					else if (cmd[2] == "file")
					{
						if (cmd[3] == "add")
						{
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_FILE, cmd[4]));
							dtn::core::BundleCore::getInstance().addConnection(n);

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " CONNECTION ADDED" << std::endl;
						}
						else if (cmd[3] == "del")
						{
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_FILE, cmd[4]));
							dtn::core::BundleCore::getInstance().removeConnection(n);

							ibrcommon::MutexLock l(_write_lock);
							(*_stream) << API_STATUS_OK << " CONNECTION REMOVED" << std::endl;
						}
					}
				}
				else if (cmd[0] == "logcat")
				{
					ibrcommon::Logger::writeBuffer(*_stream);
					(*_stream) << std::endl;
				}
				else if (cmd[0] == "bundle")
				{
					if (cmd.size() < 2) throw ibrcommon::Exception("not enough parameters");

					if (cmd[1] == "list")
					{
						// get storage object
						dtn::core::BundleCore &bcore = dtn::core::BundleCore::getInstance();
						BundleFilter filter;

						ibrcommon::MutexLock l(_write_lock);
						(*_stream) << API_STATUS_ACCEPTED << " allocated filter" << std::endl;
						std::list<dtn::data::MetaBundle> blist = bcore.getStorage().get(filter);

						for (std::list<dtn::data::MetaBundle>::const_iterator iter = blist.begin(); iter != blist.end(); iter++)
						{
							const dtn::data::MetaBundle &b = *iter;
							(*_stream) << b.toString() << std::endl;
						}
					}

//
//
//					if (cmd[1] == "get")
//					{
//						// transfer bundle data
//						ibrcommon::MutexLock l(_write_lock);
//
//						if (cmd.size() == 2)
//						{
//							(*_stream) << API_STATUS_OK << " BUNDLE GET "; sayBundleID(*_stream, _bundle_reg); (*_stream) << std::endl;
//							PlainSerializer(*_stream) << _bundle_reg;
//						}
//						else if (cmd[2] == "binary")
//						{
//							(*_stream) << API_STATUS_OK << " BUNDLE GET BINARY "; sayBundleID(*_stream, _bundle_reg); (*_stream) << std::endl;
//							dtn::data::DefaultSerializer(*_stream) << _bundle_reg; (*_stream) << std::flush;
//						}
//						else if (cmd[2] == "plain")
//						{
//							(*_stream) << API_STATUS_OK << " BUNDLE GET PLAIN "; sayBundleID(*_stream, _bundle_reg); (*_stream) << std::endl;
//							PlainSerializer(*_stream) << _bundle_reg;
//						}
//						else if (cmd[2] == "xml")
//						{
//							(*_stream) << API_STATUS_NOT_IMPLEMENTED << " FORMAT NOT IMPLEMENTED" << std::endl;
//						}
//						else
//						{
//							(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN FORMAT" << std::endl;
//						}
//					}
//					else if (cmd[1] == "put")
//					{
//						// lock the stream during reception of bundle data
//						ibrcommon::MutexLock l(_write_lock);
//
//						if (cmd.size() < 2)
//						{
//							(*_stream) << API_STATUS_BAD_REQUEST << " PLEASE DEFINE THE FORMAT" << std::endl;
//						}
//						else if (cmd[2] == "plain")
//						{
//							(*_stream) << API_STATUS_CONTINUE << " PUT BUNDLE PLAIN" << std::endl;
//
//							try {
//								PlainDeserializer(*_stream) >> _bundle_reg;
//								(*_stream) << API_STATUS_OK << " BUNDLE IN REGISTER" << std::endl;
//							} catch (const std::exception&) {
//								(*_stream) << API_STATUS_NOT_ACCEPTABLE << " PUT FAILED" << std::endl;
//
//							}
//						}
//						else if (cmd[2] == "binary")
//						{
//							(*_stream) << API_STATUS_CONTINUE << " PUT BUNDLE BINARY" << std::endl;
//
//							try {
//								dtn::data::DefaultDeserializer(*_stream) >> _bundle_reg;
//								(*_stream) << API_STATUS_OK << " BUNDLE IN REGISTER" << std::endl;
//							} catch (const std::exception&) {
//								(*_stream) << API_STATUS_NOT_ACCEPTABLE << " PUT FAILED" << std::endl;
//							}
//						}
//						else
//						{
//							(*_stream) << API_STATUS_BAD_REQUEST << " PLEASE DEFINE THE FORMAT" << std::endl;
//						}
//					}
//					else if (cmd[1] == "load")
//					{
//						if (cmd.size() < 3) throw ibrcommon::Exception("not enough parameters");
//
//						dtn::data::BundleID id;
//
//						if (cmd[2] == "queue")
//						{
//							id = _bundle_queue.getnpop();
//						}
//						else
//						{
//							// construct bundle id
//							id = readBundleID(cmd, 2);
//						}
//
//						// load the bundle
//						try {
//							_bundle_reg = dtn::core::BundleCore::getInstance().getStorage().get(id);
//
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_OK << " BUNDLE LOADED "; sayBundleID(*_stream, id); (*_stream) << std::endl;
//						} catch (const ibrcommon::Exception&) {
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_NOT_FOUND << " BUNDLE NOT FOUND" << std::endl;
//						}
//					}
//					else if (cmd[1] == "clear")
//					{
//						_bundle_reg = dtn::data::Bundle();
//
//						ibrcommon::MutexLock l(_write_lock);
//						(*_stream) << API_STATUS_OK << " BUNDLE CLEARED" << std::endl;
//					}
//					else if (cmd[1] == "free")
//					{
//						try {
//							dtn::core::BundleCore::getInstance().getStorage().remove(_bundle_reg);
//							_bundle_reg = dtn::data::Bundle();
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_OK << " BUNDLE FREE SUCCESSFUL" << std::endl;
//						} catch (const ibrcommon::Exception&) {
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_NOT_FOUND << " BUNDLE NOT FOUND" << std::endl;
//						}
//					}
//					else if (cmd[1] == "delivered")
//					{
//						if (cmd.size() < 3) throw ibrcommon::Exception("not enough parameters");
//
//						try {
//							// construct bundle id
//							dtn::data::BundleID id = readBundleID(cmd, 2);
//							dtn::data::MetaBundle meta = dtn::core::BundleCore::getInstance().getStorage().get(id);
//
//							// raise bundle event
//							dtn::core::BundleEvent::raise(meta, BUNDLE_DELIVERED);
//
//							// delete it if it was a singleton
//							if (meta.get(dtn::data::PrimaryBlock::DESTINATION_IS_SINGLETON))
//							{
//								dtn::core::BundleCore::getInstance().getStorage().remove(id);
//							}
//
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_OK << " BUNDLE DELIVERED ACCEPTED" << std::endl;
//						} catch (const ibrcommon::Exception&) {
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_NOT_FOUND << " BUNDLE NOT FOUND" << std::endl;
//						}
//					}
//					else if (cmd[1] == "store")
//					{
//						// store the bundle in the storage
//						try {
//							dtn::core::BundleCore::getInstance().getStorage().store(_bundle_reg);
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_OK << " BUNDLE STORE SUCCESSFUL" << std::endl;
//						} catch (const ibrcommon::Exception&) {
//							ibrcommon::MutexLock l(_write_lock);
//							(*_stream) << API_STATUS_INTERNAL_ERROR << " BUNDLE STORE FAILED" << std::endl;
//						}
//					}
//					else if (cmd[1] == "send")
//					{
//						processIncomingBundle(_bundle_reg);
//
//						ibrcommon::MutexLock l(_write_lock);
//						(*_stream) << API_STATUS_OK << " BUNDLE SENT" << std::endl;
//					}
//					else
//					{
//						ibrcommon::MutexLock l(_write_lock);
//						(*_stream) << API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
//					}
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
}
