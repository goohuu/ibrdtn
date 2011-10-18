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
#include "api/ManagementConnection.h"
#include "api/EventConnection.h"
#include "api/ExtendedApiHandler.h"
#include "core/BundleCore.h"
#include <ibrcommon/Logger.h>
#include <ibrdtn/utils/Utils.h>
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
					_handler->finally();
					delete _handler;
					_handler = NULL;

					// end this stream, return to the previous stage
					(*_stream) << ClientHandler::API_STATUS_OK << " SWITCHED TO LEVEL 0" << std::endl;

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
						else if (cmd[1] == "management")
						{
							// switch to the management protocol
							_handler = new ManagementConnection(*this, *_stream);
							continue;
						}
						else if (cmd[1] == "event")
						{
							// switch to the management protocol
							_handler = new EventConnection(*this, *_stream);
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
