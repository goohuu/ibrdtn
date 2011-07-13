
#ifndef IBRDTN_DAEMON_NODE_H_
#define IBRDTN_DAEMON_NODE_H_

#include <string>
#include <ibrdtn/data/EID.h>
#include <map>
#include <set>
#include <list>

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
			/**
			 * Specify a node type.
			 * FLOATING is a node, if it's not statically reachable.
			 * PERMANENT is used for static nodes with are permanently reachable.
			 */
			enum Type
			{
				NODE_FLOATING = 0,
				NODE_PERMANENT = 1,
				NODE_CONNECTED = 2
			};

			enum Protocol
			{
				CONN_UNSUPPORTED = -1,
				CONN_UNDEFINED = 0,
				CONN_UDPIP = 1,
				CONN_TCPIP = 2,
				CONN_LOWPAN = 3,
				CONN_BLUETOOTH = 4,
				CONN_HTTP = 5
			};

			class URI
			{
			public:
				URI(const std::string &uri, const Protocol p);
				~URI();

				const Protocol protocol;
				const std::string value;

				void decode(std::string &address, unsigned int &port) const;

				bool operator<(const URI &other) const;
				bool operator==(const URI &other) const;

				bool operator==(const Node::Protocol &p) const;
			};

			class Attribute
			{
			public:
				Attribute(const std::string &name, const std::string &value);
				~Attribute();

				const std::string name;
				const std::string value;

				bool operator<(const Attribute &other) const;
				bool operator==(const Attribute &other) const;

				bool operator==(const std::string &name) const;
			};

			static std::string getTypeName(Node::Type type);
			static std::string getProtocolName(Node::Protocol proto);

			/**
			 * constructor
			 * @param type set the node type
			 * @sa NoteType
			 */
			Node(Node::Type type = NODE_PERMANENT, unsigned int rtt = 2700);

			Node(const dtn::data::EID &id, Node::Type type = NODE_PERMANENT, unsigned int rtt = 2700);

			/**
			 * destructor
			 */
			virtual ~Node();

			/**
			 * get the node type.
			 * @return The type of the node.
			 * @sa NoteType
			 */
			Node::Type getType() const;
			void setType(Node::Type type);

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

			/*
			 * Set a description for the node.
			 * @param description a description
			 */
			void setDescription(std::string description);

			/**
			 * Get the assigned description for the node.
			 * @return The description.
			 */
			std::string getDescription() const;

			/**
			 * Set the EID of this Node
			 * @param id The EID to set.
			 */
			void setEID(const dtn::data::EID &id);

			/**
			 * Return the EID of this node.
			 * @return The EID of this node.
			 */
			const dtn::data::EID& getEID() const;

			/**
			 * Set the timeout of the node. This is only relevant if the node is of type
			 * FLOATING. The node get expired if the timeout if count to zero.
			 * @param timeout The timeout in seconds.
			 */
			void setTimeout(int timeout);

			/**
			 * Get the current timeout.
			 * @return The timeout in seconds.
			 */
			int getTimeout() const;

			/**
			 * Get the roundtrip time for this contact
			 */
			unsigned int getRoundTripTime() const;

			/**
			 * Decrement the timeout by <step>.
			 * @param step Decrement the timeout by this value.
			 * @return false, if the timeout steps below zero. Timeout is reached.
			 */
			bool decrementTimeout(int step);

			/**
			 * Compare this node to another one. Two nodes are equal if the
			 * uri and address of both nodes are equal.
			 * @param node A other node to compare.
			 * @return true, if the given node is equal to this node.
			 */
			bool operator==(const Node &other) const;
			bool operator<(const Node &other) const;

			std::string toString() const;

			bool doConnectImmediately() const;
			void setConnectImmediately(bool val);

		private:
			bool _connect_immediately;
			std::string _description;
			dtn::data::EID _id;
			int _timeout;
			unsigned int _rtt;
			Node::Type _type;

			std::set<URI> _uri_list;
			std::set<Attribute> _attr_list;
			std::map<std::string, std::string> _attributes;
		};
	}
}

#endif /*IBRDTN_DAEMON_NODE_H_*/
