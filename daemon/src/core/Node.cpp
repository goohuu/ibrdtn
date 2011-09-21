#include "core/Node.h"
#include "net/ConvergenceLayer.h"
#include <ibrdtn/utils/Clock.h>
#include <ibrdtn/utils/Utils.h>
#include <ibrcommon/Logger.h>

#include <iostream>
#include <sstream>

using namespace std;

namespace dtn
{
	namespace core
	{
		Node::URI::URI(const Node::Type t, const Node::Protocol p, const std::string &uri, const size_t to)
		 : type(t), protocol(p), value(uri), expire((to == 0) ? 0 : dtn::utils::Clock::getTime() + to)
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

			if (type < other.type) return true;
			if (type != other.type) return false;

			return (value < other.value);
		}

		bool Node::URI::operator==(const URI &other) const
		{
			return ((type == other.type) && (protocol == other.protocol) && (value == other.value));
		}

		bool Node::URI::operator==(const Node::Protocol &p) const
		{
			return (protocol == p);
		}

		bool Node::URI::operator==(const Node::Type &t) const
		{
			return (type == t);
		}

		std::ostream& operator<<(std::ostream &stream, const Node::URI &u)
		{
			stream << Node::toString(u.type) << "#" << Node::toString(u.protocol) << "#" << u.value;
			return stream;
		}

		Node::Attribute::Attribute(const Type t, const std::string &n, const std::string &v, const size_t to)
		 : type(t), name(n), value(v), expire((to == 0) ? 0 : dtn::utils::Clock::getTime() + to)
		{
		}

		Node::Attribute::~Attribute()
		{
		}

		bool Node::Attribute::operator<(const Attribute &other) const
		{
			if (name < other.name) return true;
			if (name != other.name) return false;

			return (type < other.type);
		}

		bool Node::Attribute::operator==(const Attribute &other) const
		{
			return ((type == other.type) && (name == other.name));
		}

		bool Node::Attribute::operator==(const std::string &n) const
		{
			return (name == n);
		}

		std::ostream& operator<<(std::ostream &stream, const Node::Attribute &a)
		{
			stream << Node::toString(a.type) << "#" << a.name << "#" << a.value;
			return stream;
		}

		Node::Node(const dtn::data::EID &id)
		: _connect_immediately(false), _id(id)
		{
		}

		Node::~Node()
		{
		}

		std::string Node::toString(Node::Type type)
		{
			switch (type)
			{
			case Node::NODE_UNAVAILABLE:
				return "unavailable";

			case Node::NODE_DISCOVERED:
				return "discovered";

			case Node::NODE_STATIC:
				return "static";

			case Node::NODE_CONNECTED:
				return "connected";
			}

			return "unknown";
		}

		std::string Node::toString(Node::Protocol proto)
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

			case Node::CONN_FILE:
				return "FILE";
			}

			return "unknown";
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
			_uri_list.erase(u);
			_uri_list.insert(u);
		}

		void Node::add(const Attribute &attr)
		{
			_attr_list.erase(attr);
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

		std::list<Node::URI> Node::get(Node::Type type, Node::Protocol proto) const
		{
			std::list<URI> ret;
			for (std::set<URI>::const_iterator iter = _uri_list.begin(); iter != _uri_list.end(); iter++)
			{
				if (((*iter) == proto) && ((*iter) == type)) ret.push_back(*iter);
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

		const dtn::data::EID& Node::getEID() const
		{
			return _id;
		}

		/**
		 *
		 * @return True, if all attributes are gone
		 */
		bool Node::expire()
		{
			// get the current timestamp
			size_t ct = dtn::utils::Clock::getTime();

			// walk though all Attribute elements and remove the expired ones
			{
				std::set<Attribute>::const_iterator iter = _attr_list.begin();
				while ( iter != _attr_list.end() )
				{
					const Attribute &attr = (*iter);
					if ((attr.expire > 0) && (attr.expire < ct))
					{
						// remove this attribute
						_attr_list.erase(iter++);
					}
					else
					{
						iter++;
					}
				}
			}

			// walk though all URI elements and remove the expired ones
			{
				std::set<URI>::const_iterator iter = _uri_list.begin();
				while ( iter != _uri_list.end() )
				{
					const URI &u = (*iter);
					if ((u.expire > 0) && (u.expire < ct))
					{
						// remove this attribute
						_uri_list.erase(iter++);
					}
					else
					{
						iter++;
					}
				}
			}

			return (_attr_list.empty() && _uri_list.empty());
		}

		const Node& Node::operator+=(const Node &other)
		{
			for (std::set<Attribute>::const_iterator iter = other._attr_list.begin(); iter != other._attr_list.end(); iter++)
			{
				const Attribute &attr = (*iter);
				add(attr);
			}

			for (std::set<URI>::const_iterator iter = other._uri_list.begin(); iter != other._uri_list.end(); iter++)
			{
				const URI &u = (*iter);
				add(u);
			}

			return (*this);
		}

		const Node& Node::operator-=(const Node &other)
		{
			for (std::set<Attribute>::const_iterator iter = other._attr_list.begin(); iter != other._attr_list.end(); iter++)
			{
				const Attribute &attr = (*iter);
				remove(attr);
			}

			for (std::set<URI>::const_iterator iter = other._uri_list.begin(); iter != other._uri_list.end(); iter++)
			{
				const URI &u = (*iter);
				remove(u);
			}

			return (*this);
		}

		bool Node::operator==(const dtn::data::EID &other) const
		{
			return (other == _id);
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
			std::stringstream ss; ss << getEID().getString();
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

		bool Node::isAvailable() const
		{
			return !_uri_list.empty();
		}

		std::ostream& operator<<(std::ostream &stream, const Node &node)
		{
			stream << "Node: " << node._id.getString() << " [ ";
			for (std::set<Node::Attribute>::const_iterator iter = node._attr_list.begin(); iter != node._attr_list.end(); iter++)
			{
				const Node::Attribute &attr = (*iter);
				stream << attr << "#expire=" << attr.expire << "; ";
			}

			for (std::set<Node::URI>::const_iterator iter = node._uri_list.begin(); iter != node._uri_list.end(); iter++)
			{
				const Node::URI &u = (*iter);
				stream << u << "#expire=" << u.expire << "; ";
			}
			stream << " ]";

			return stream;
		}
	}
}
