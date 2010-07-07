#include "core/Node.h"
#include "net/ConvergenceLayer.h"

#include <iostream>

using namespace std;

namespace dtn
{
	namespace core
	{
		Node::Node(Node::Type type, unsigned int rtt)
		: _address(), _description(), _id("dtn:none"), _timeout(5), _rtt(rtt), _type(type), _port(4556), _protocol(CONN_UNDEFINED)
		{
		}

		Node::Node(dtn::data::EID id, Node::Protocol proto, Node::Type type, unsigned int rtt)
		: _address(), _description(), _id(id), _timeout(5), _rtt(rtt), _type(type), _port(4556), _protocol(proto)
		{

		}

		Node::~Node()
		{
		}

		std::string Node::getTypeName(Node::Type type)
		{
			switch (type)
			{
			case Node::NODE_FLOATING:
				return "floating";

			case Node::NODE_PERMANENT:
				return "permanent";
			}

			return "unknown";
		}

		std::string Node::getProtocolName(Node::Protocol proto)
		{
			switch (proto)
			{
			case Node::CONN_UNSUPPORTED:
				return "unsupported";

			case Node::CONN_UNDEFINED:
				return "undefined";

			case Node::CONN_UDPIP:
				return "UDP";

			case Node::CONN_TCPIP:
				return "TCP";

			case Node::CONN_ZIGBEE:
				return "ZigBee";

			case Node::CONN_BLUETOOTH:
				return "Bluetooth";
			}

			return "unknown";
		}

		Node::Type Node::getType() const
		{
			return _type;
		}

		void Node::setProtocol(Node::Protocol protocol)
		{
			_protocol = protocol;
		}

		Node::Protocol Node::getProtocol() const
		{
			return _protocol;
		}

		void Node::setAddress(string address)
		{
			_address = address;
		}

		string Node::getAddress() const
		{
			return _address;
		}

		void Node::setPort(unsigned int port)
		{
			_port = port;
		}

		unsigned int Node::getPort() const
		{
			return _port;
		}

		void Node::setDescription(string description)
		{
			_description = description;
		}

		string Node::getDescription() const
		{
			return _description;
		}


		void Node::setURI(string uri)
		{
			_id = dtn::data::EID(uri);
		}

		string Node::getURI() const
		{
			return _id.getString();
		}


		void Node::setTimeout(int timeout)
		{
			_timeout = timeout;
		}

		int Node::getTimeout() const
		{
			return _timeout;
		}

		unsigned int Node::getRoundTripTime() const
		{
			return _rtt;
		}

		bool Node::decrementTimeout(int step)
		{
			if (_type == NODE_PERMANENT) return true;

			if (_timeout <= 0) return false;
			_timeout -= step;
			return true;
		}

		bool Node::operator==(const Node &other) const
		{
			return (other._id == _id) && (other._protocol == _protocol);
		}

		int Node::operator<(const Node &other) const
		{
			if (_id < other._id) return true;
			if (_protocol < other._protocol) return true;
			return false;
		}
	}
}
