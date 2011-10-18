/*
 * ManagementConnection.cpp
 *
 *  Created on: 13.10.2011
 *      Author: morgenro
 */

#include "config.h"
#include "Configuration.h"
#include "ManagementConnection.h"
#include "core/BundleCore.h"

#include <ibrdtn/utils/Utils.h>

#include <ibrcommon/Logger.h>
#include <ibrcommon/net/LinkManager.h>

namespace dtn
{
	namespace api
	{
		ManagementConnection::ManagementConnection(ClientHandler &client, ibrcommon::tcpstream &stream)
		 : ProtocolHandler(client, stream)
		{
		}

		ManagementConnection::~ManagementConnection()
		{
		}

		void ManagementConnection::run()
		{
			std::string buffer;
			_stream << ClientHandler::API_STATUS_OK << " SWITCHED TO MANAGEMENT" << std::endl;

			// run as long the stream is ok
			while (_stream.good())
			{
				getline(_stream, buffer);

				// search for '\r\n' and remove the '\r'
				std::string::reverse_iterator iter = buffer.rbegin();
				if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);

				std::vector<std::string> cmd = dtn::utils::Utils::tokenize(" ", buffer);
				if (cmd.size() == 0) continue;

				if (cmd[0] == "exit")
				{
					// return to previous level
					break;
				}
				else
				{
					// forward to standard command set
					processCommand(cmd);
				}
			}
		}

		void ManagementConnection::finally()
		{
		}

		void ManagementConnection::setup()
		{
		}

		bool ManagementConnection::__cancellation()
		{
			return true;
		}

		void ManagementConnection::processCommand(const std::vector<std::string> &cmd)
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
				if (cmd[0] == "neighbor")
				{
					if (cmd.size() < 2) throw ibrcommon::Exception("not enough parameters");

					if (cmd[1] == "list")
					{
						const std::set<dtn::core::Node> nlist = dtn::core::BundleCore::getInstance().getNeighbors();

						_stream << ClientHandler::API_STATUS_OK << " NEIGHBOR LIST" << std::endl;
						for (std::set<dtn::core::Node>::const_iterator iter = nlist.begin(); iter != nlist.end(); iter++)
						{
							_stream << (*iter).getEID().getString() << std::endl;
						}
						_stream << std::endl;
					}
					else
					{
						_stream << ClientHandler::API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
					}
				}
				else if (cmd[0] == "interface")
				{
					if (cmd[1] == "address")
					{
						try {
							ibrcommon::LinkManager &lm = ibrcommon::LinkManager::getInstance();

							ibrcommon::vaddress::Family f = ibrcommon::vaddress::VADDRESS_UNSPEC;

							if (cmd[4] == "ipv4")	f = ibrcommon::vaddress::VADDRESS_INET;
							if (cmd[4] == "ipv6")	f = ibrcommon::vaddress::VADDRESS_INET6;

							ibrcommon::vinterface iface(cmd[3]);
							ibrcommon::vaddress addr(f, cmd[5]);

							if (cmd[2] == "add")
							{
								lm.addressAdded(iface, addr);
								_stream << ClientHandler::API_STATUS_OK << " ADDRESS ADDED" << std::endl;

							}
							else if (cmd[2] == "del")
							{
								lm.addressRemoved(iface, addr);
								_stream << ClientHandler::API_STATUS_OK << " ADDRESS REMOVED" << std::endl;
							}
							else
							{
								_stream << ClientHandler::API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
							}
						} catch (const std::bad_cast&) {
							_stream << ClientHandler::API_STATUS_NOT_ALLOWED << " FEATURE NOT AVAILABLE" << std::endl;
						};
					}
					else
					{
						_stream << ClientHandler::API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
					}
				}
				else if (cmd[0] == "connection")
				{
					// need to process the connection arguments
					// the arguments look like:
					// <eid> [tcp] [add|del] <ip> <port>
					dtn::core::Node n(cmd[1]);

					if (cmd[2] == "tcp")
					{
						if (cmd[3] == "add")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_TCPIP, uri));
							dtn::core::BundleCore::getInstance().addConnection(n);

							_stream << ClientHandler::API_STATUS_OK << " CONNECTION ADDED" << std::endl;
						}
						else if (cmd[3] == "del")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_TCPIP, uri));
							dtn::core::BundleCore::getInstance().removeConnection(n);

							_stream << ClientHandler::API_STATUS_OK << " CONNECTION REMOVED" << std::endl;
						}
					}
					else if (cmd[2] == "udp")
					{
						if (cmd[3] == "add")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_UDPIP, uri));
							dtn::core::BundleCore::getInstance().addConnection(n);

							_stream << ClientHandler::API_STATUS_OK << " CONNECTION ADDED" << std::endl;
						}
						else if (cmd[3] == "del")
						{
							std::string uri = "ip=" + cmd[4] + ";port=" + cmd[5] + ";";
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_UDPIP, uri));
							dtn::core::BundleCore::getInstance().removeConnection(n);

							_stream << ClientHandler::API_STATUS_OK << " CONNECTION REMOVED" << std::endl;
						}
					}
					else if (cmd[2] == "file")
					{
						if (cmd[3] == "add")
						{
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_FILE, cmd[4]));
							dtn::core::BundleCore::getInstance().addConnection(n);

							_stream << ClientHandler::API_STATUS_OK << " CONNECTION ADDED" << std::endl;
						}
						else if (cmd[3] == "del")
						{
							n.add(dtn::core::Node::URI(dtn::core::Node::NODE_STATIC, dtn::core::Node::CONN_FILE, cmd[4]));
							dtn::core::BundleCore::getInstance().removeConnection(n);

							_stream << ClientHandler::API_STATUS_OK << " CONNECTION REMOVED" << std::endl;
						}
					}
				}
				else if (cmd[0] == "logcat")
				{
					ibrcommon::Logger::writeBuffer(_stream);
					_stream << std::endl;
				}
				else if (cmd[0] == "bundle")
				{
					if (cmd[1] == "list")
					{
						// get storage object
						dtn::core::BundleCore &bcore = dtn::core::BundleCore::getInstance();
						BundleFilter filter;

						_stream << ClientHandler::API_STATUS_ACCEPTED << " allocated filter" << std::endl;
						std::list<dtn::data::MetaBundle> blist = bcore.getStorage().get(filter);

						for (std::list<dtn::data::MetaBundle>::const_iterator iter = blist.begin(); iter != blist.end(); iter++)
						{
							const dtn::data::MetaBundle &b = *iter;
							_stream << b.toString() << std::endl;
						}
					}
				}
				else
				{
					_stream << ClientHandler::API_STATUS_BAD_REQUEST << " UNKNOWN COMMAND" << std::endl;
				}
			} catch (const std::exception&) {
				_stream << ClientHandler::API_STATUS_BAD_REQUEST << " ERROR" << std::endl;
			}
		}
	} /* namespace api */
} /* namespace dtn */
