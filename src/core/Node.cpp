#include "core/Node.h"
#include "net/ConvergenceLayer.h"

#include <iostream>
using namespace std;

namespace dtn
{
	namespace core
	{
		Node::Node(NodeType type, unsigned int rtt)
		: m_address("dtn:unknown"), m_timeout(5), m_rtt(rtt), m_type(type), m_port(4556), _protocol(UNDEFINED)
		{
		}

		Node::Node(const Node &k)
		: m_address(k.m_address), m_timeout(k.m_timeout), m_type(k.m_type), m_port(k.m_port),
		m_description(k.m_description), m_uri(k.m_uri), _protocol(k._protocol)
		{

		}

		Node::~Node()
		{
		}

		NodeType Node::getType() const
		{
			return m_type;
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
			m_address = address;
		}

		string Node::getAddress() const
		{
			return m_address;
		}

		void Node::setPort(unsigned int port)
		{
			m_port = port;
		}

		unsigned int Node::getPort() const
		{
			return m_port;
		}

		void Node::setDescription(string description)
		{
			m_description = description;
		}

		string Node::getDescription() const
		{
			return m_description;
		}


		void Node::setURI(string uri)
		{
			m_uri = uri;
		}

		string Node::getURI() const
		{
			return m_uri;
		}


		void Node::setTimeout(int timeout)
		{
			m_timeout = timeout;
		}

		int Node::getTimeout() const
		{
			return m_timeout;
		}

		unsigned int Node::getRoundTripTime() const
		{
			return m_rtt;
		}

		bool Node::decrementTimeout(int step)
		{
			if (m_type == PERMANENT) return true;

			if (m_timeout <= 0) return false;
			m_timeout -= step;
			return true;
		}

		bool Node::equals(const Node &node) const
		{
			if (node.getURI() != getURI()) return false;
			if (node.getProtocol() != getProtocol()) return false;
			// TODO: enable after testing
			//if (node.getAddress() != getAddress()) return false;
			return true;
		}
	}
}
