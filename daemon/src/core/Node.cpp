#include "core/Node.h"
#include "net/ConvergenceLayer.h"

#include <iostream>

using namespace std;

namespace dtn
{
	namespace core
	{
		Node::Node(NodeType type, unsigned int rtt)
		: _address(), _description(), _id("dtn:none"), _timeout(5), _rtt(rtt), _type(type), _port(4556), _protocol(UNDEFINED)
		{
		}

		Node::Node(dtn::data::EID id, NodeProtocol proto, NodeType type, unsigned int rtt)
		: _address(), _description(), _id(id), _timeout(5), _rtt(rtt), _type(type), _port(4556), _protocol(proto)
		{

		}

		Node::~Node()
		{
		}

		NodeType Node::getType() const
		{
			return _type;
		}

		void Node::setProtocol(NodeProtocol protocol)
		{
			_protocol = protocol;
		}

		NodeProtocol Node::getProtocol() const
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
			if (_type == PERMANENT) return true;

			if (_timeout <= 0) return false;
			_timeout -= step;
			return true;
		}

		bool Node::operator==(const Node &other) const
		{
			if (other.getURI() != getURI()) return false;

			if (other.getProtocol() != UNDEFINED)
			{
				if (other.getProtocol() != getProtocol()) return false;
			}

			// TODO: enable after testing
			//if (other.getAddress() != getAddress()) return false;
			return true;
		}

		int Node::operator<(const Node &other) const
		{
			if (getURI() < other.getURI()) return true;
			if (getProtocol() < other.getProtocol()) return true;
			// TODO: enable after testing
			//if (other.getAddress() >= getAddress()) return false;
			return false;
		}
	}
}
