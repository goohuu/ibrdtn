
#ifndef IBRDTN_DAEMON_NODE_H_
#define IBRDTN_DAEMON_NODE_H_

#include <string>
#include <ibrdtn/data/EID.h>
#include <set>
#include <list>
#include <iostream>

namespace dtn
{
	namespace net
	{
		class ConvergenceLayer;
	}

	namespace core
	{
		/**
		 * A Node instance holds all informations of a neighboring node.
		 */
		class Node
		{
		public:
			enum Protocol
			{
				CONN_UNSUPPORTED = -1,
				CONN_UNDEFINED = 0,
				CONN_TCPIP = 1,
				CONN_UDPIP = 2,
				CONN_LOWPAN = 3,
				CONN_BLUETOOTH = 4,
				CONN_HTTP = 5
			};

			/**
			 * Specify a node type.
			 * FLOATING is a node, if it's not statically reachable.
			 * PERMANENT is used for static nodes with are permanently reachable.
			 */
			enum Type
			{
				NODE_UNAVAILABLE = 0,
				NODE_CONNECTED = 1,
				NODE_DISCOVERED = 2,
				NODE_STATIC = 3
			};

			class URI
			{
			public:
				URI(const Type t, const Protocol proto, const std::string &uri, const size_t timeout = 0);
				~URI();

				const Type type;
				const Protocol protocol;
				const std::string value;
				const size_t expire;

				void decode(std::string &address, unsigned int &port) const;

				bool operator<(const URI &other) const;
				bool operator==(const URI &other) const;

				bool operator==(const Node::Protocol &p) const;

				friend std::ostream& operator<<(std::ostream&, const Node::URI&);
			};

			class Attribute
			{
			public:
				Attribute(const Type t, const std::string &name, const std::string &value, const size_t timeout = 0);
				~Attribute();

				const Type type;
				const std::string name;
				const std::string value;
				const size_t expire;

				bool operator<(const Attribute &other) const;
				bool operator==(const Attribute &other) const;

				bool operator==(const std::string &name) const;

				friend std::ostream& operator<<(std::ostream&, const Node::Attribute&);
			};

			static std::string toString(Node::Type type);
			static std::string toString(Node::Protocol proto);

			/**
			 * constructor
			 * @param type set the node type
			 * @sa NoteType
			 */
			Node(const dtn::data::EID &id);

			/**
			 * destructor
			 */
			virtual ~Node();

			/**
			 * Check if the protocol is available for this node.
			 * @param proto
			 * @return
			 */
			bool has(Node::Protocol proto) const;
			bool has(const std::string &name) const;

			/**
			 * Add a URI to this node.
			 * @param u
			 */
			void add(const URI &u);
			void add(const Attribute &attr);

			/**
			 * Remove a given URI of the node.
			 * @param proto
			 */
			void remove(const URI &u);
			void remove(const Attribute &attr);

			/**
			 * Clear all URIs & Attributes contained in this node.
			 */
			void clear();

			/**
			 * Returns a list of URIs matching the given protocol
			 * @param proto
			 * @return
			 */
			std::list<URI> get(Node::Protocol proto) const;

			/**
			 * get a list of attributes match the given name
			 * @param name
			 * @return
			 */
			std::list<Attribute> get(const std::string &name) const;

			/**
			 * Return the EID of this node.
			 * @return The EID of this node.
			 */
			const dtn::data::EID& getEID() const;

			/**
			 * remove expired service descriptors
			 */
			bool expire();

			/**
			 * Compare this node to another one. Two nodes are equal if the
			 * uri and address of both nodes are equal.
			 * @param node A other node to compare.
			 * @return true, if the given node is equal to this node.
			 */
			bool operator==(const Node &other) const;
			bool operator<(const Node &other) const;

			bool operator==(const dtn::data::EID &other) const;

			const Node& operator+=(const Node &other);
			const Node& operator-=(const Node &other);

			std::string toString() const;

			bool doConnectImmediately() const;
			void setConnectImmediately(bool val);

			/**
			 * returns true, if at least one connection is available
			 */
			bool isAvailable() const;

			friend std::ostream& operator<<(std::ostream&, const Node&);

		private:
			bool _connect_immediately;
			dtn::data::EID _id;

			std::set<URI> _uri_list;
			std::set<Attribute> _attr_list;
		};
	}
}

#endif /*IBRDTN_DAEMON_NODE_H_*/
