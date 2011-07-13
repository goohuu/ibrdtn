#include "core/Node.h"
#include "net/ConvergenceLayer.h"
#include <ibrdtn/utils/Utils.h>
#include <ibrcommon/Logger.h>

#include <iostream>
#include <sstream>

using namespace std;

namespace dtn
{
	namespace core
	{
		Node::URI::URI(const std::string &uri, const Protocol p)
		 : protocol(p), value(uri)
		{
		}

		Node::URI::~URI()
		{
		}

		void Node::URI::decode(std::string &address, unsigned int &port) const
		{
			// parse parameters
			std::vector<string> parameters = dtn::utils::Utils::tokenize(";", value);
			std::vector<string>::const_iterator param_iter = parameters.begin();

			while (param_iter != parameters.end())
			{
				std::vector<string> p = dtn::utils::Utils::tokenize("=", (*param_iter));

				if (p[0].compare("ip") == 0)
				{
					address = p[1];
				}

				if (p[0].compare("port") == 0)
				{
					std::stringstream port_stream;
					port_stream << p[1];
					port_stream >> port;
				}

				param_iter++;
			}
		}

		bool Node::URI::operator<(const URI &other) const
		{
			if (protocol < other.protocol) return true;
			if (protocol != other.protocol) return false;

			return (value < other.value);
		}

		bool Node::URI::operator==(const URI &other) const
		{
			return ((protocol == other.protocol) && (value == other.value));
		}

		bool Node::URI::operator==(const Node::Protocol &p) const
		{
			return (protocol == p);
		}

		Node::Attribute::Attribute(const std::string &n, const std::string &v)
		 : name(n), value(v)
		{
		}

		Node::Attribute::~Attribute()
		{
		}

		bool Node::Attribute::operator<(const Attribute &other) const
		{
			if (name < other.name) return true;
			if (name != other.name) return false;

			return (value < other.value);
		}

		bool Node::Attribute::operator==(const Attribute &other) const
		{
			return ((name == other.name) && (value == other.value));
		}

		bool Node::Attribute::operator==(const std::string &n) const
		{
			return (name == n);
		}

		Node::Node(Node::Type type, unsigned int rtt)
		: _connect_immediately(false), _description(), _id("dtn:none"), _timeout(5), _rtt(rtt), _type(type)
		{
		}

		Node::Node(const dtn::data::EID &id, Node::Type type, unsigned int rtt)
		: _connect_immediately(false), _description(), _id(id), _timeout(5), _rtt(rtt), _type(type)
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

			case Node::NODE_CONNECTED:
				return "connected";
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

			case Node::CONN_LOWPAN:
				return "LoWPAN";

			case Node::CONN_BLUETOOTH:
				return "Bluetooth";

			case Node::CONN_HTTP:
				return "HTTP";
			}

			return "unknown";
		}

		Node::Type Node::getType() const
		{
			return _type;
		}

		void Node::setType(Node::Type type)
		{
			_type = type;
		}

		bool Node::has(Node::Protocol proto) const
		{
			for (std::set<URI>::const_iterator iter = _uri_list.begin(); iter != _uri_list.end(); iter++)
			{
				if ((*iter) == proto) return true;
			}
			return false;
		}

		bool Node::has(const std::string &name) const
		{
			for (std::set<Attribute>::const_iterator iter = _attr_list.begin(); iter != _attr_list.end(); iter++)
			{
				if ((*iter) == name) return true;
			}
			return false;
		}

		void Node::add(const URI &u)
		{
			_uri_list.insert(u);
		}

		void Node::add(const Attribute &attr)
		{
			_attr_list.insert(attr);
		}

		void Node::remove(const URI &u)
		{
			_uri_list.erase(u);
		}

		void Node::remove(const Attribute &attr)
		{
			_attr_list.erase(attr);
		}

		void Node::clear()
		{
			_uri_list.clear();
			_attr_list.clear();
		}

		std::list<Node::URI> Node::get(Node::Protocol proto) const
		{
			std::list<URI> ret;
			for (std::set<URI>::const_iterator iter = _uri_list.begin(); iter != _uri_list.end(); iter++)
			{
				if ((*iter) == proto) ret.push_back(*iter);
			}
			return ret;
		}

		std::list<Node::Attribute> Node::get(const std::string &name) const
		{
			std::list<Attribute> ret;
			for (std::set<Attribute>::const_iterator iter = _attr_list.begin(); iter != _attr_list.end(); iter++)
			{
				if ((*iter) == name) ret.push_back(*iter);
			}
			return ret;
		}

		void Node::setDescription(string description)
		{
			_description = description;
		}

		string Node::getDescription() const
		{
			return _description;
		}

		void Node::setEID(const dtn::data::EID &id)
		{
			_id = id;
		}

		const dtn::data::EID& Node::getEID() const
		{
			return _id;
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
			return (other._id == _id);
		}

		bool Node::operator<(const Node &other) const
		{
			if (_id < other._id ) return true;

			return false;
		}

		std::string Node::toString() const
		{
			std::stringstream ss; ss << getEID().getString() << " (" << Node::getTypeName(getType()) << ")";
			return ss.str();
		}

		bool Node::doConnectImmediately() const
		{
			return _connect_immediately;
		}

		void Node::setConnectImmediately(bool val)
		{
			_connect_immediately = val;
		}
	}
}
