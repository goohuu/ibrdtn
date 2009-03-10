#include "core/Node.h"
#include "core/ConvergenceLayer.h"

#include <iostream>
using namespace std;

namespace dtn
{
namespace core
{

Node::Node(NodeType type)
: m_address("dtn:unknown"), m_type(type), m_port(4556), m_cl(NULL)
{
}

Node::Node(const Node &k)
: m_address(k.m_address), m_type(k.m_type), m_port(k.m_port), m_cl(k.m_cl),
m_description(k.m_description), m_position(k.m_position), m_uri(k.m_uri)
{

}

Node::~Node()
{
}

NodeType Node::getType() const
{
	return m_type;
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

bool Node::decrementTimeout(int step)
{
	if (m_type == PERMANENT) return true;

	if (m_timeout <= 0) return false;
	m_timeout -= step;
	return true;
}

bool Node::equals(Node &node)
{
	if (node.getURI() != getURI()) return false;
	if (node.getAddress() != getAddress()) return false;
	return true;
}

ConvergenceLayer* Node::getConvergenceLayer() const
{
	return m_cl;
}

void Node::setConvergenceLayer(ConvergenceLayer *cl)
{
	m_cl = cl;
}

void Node::setPosition(pair<double,double> value)
{
	m_position = value;
}

pair<double,double> Node::getPosition() const
{
	return m_position;
}

}
}
