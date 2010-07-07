
#ifndef IBRDTN_DAEMON_NODE_H_
#define IBRDTN_DAEMON_NODE_H_

#include <string>
#include <ibrdtn/data/EID.h>

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
				NODE_PERMANENT = 1
			};

			enum Protocol
			{
				CONN_UNSUPPORTED = -1,
				CONN_UNDEFINED = 0,
				CONN_UDPIP = 1,
				CONN_TCPIP = 2,
				CONN_ZIGBEE = 3,
				CONN_BLUETOOTH = 4
			};

			static std::string getTypeName(Node::Type type);
			static std::string getProtocolName(Node::Protocol proto);

			/**
			 * constructor
			 * @param type set the node type
			 * @sa NoteType
			 */
			Node(Node::Type type = NODE_PERMANENT, unsigned int rtt = 2700);

			Node(dtn::data::EID id, Node::Protocol proto = CONN_UNDEFINED, Node::Type type = NODE_PERMANENT, unsigned int rtt = 2700);

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

			void setProtocol(Node::Protocol protocol);
			Node::Protocol getProtocol() const;

			/**
			 * Set the address of the node.
			 * @param address The address of the node. (e.g. 10.0.0.1 for IP)
			 */
			void setAddress(std::string address);

			/**
			 * Get the address of the node.
			 * @return The address of the node. (e.g. 10.0.0.1 for IP)
			 */
			std::string getAddress() const;

			void setPort(unsigned int port);
			unsigned int getPort() const;

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
			 * Set the URI (EID) of the node.
			 * @param uri The URI of the node.
			 */
			void setURI(std::string uri);

			/**
			 * Get the URI (EID) of the node.
			 * @return The URI of the node.
			 */
			std::string getURI() const;

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

		private:
			std::string _address;
			std::string _description;
			dtn::data::EID _id;
			int _timeout;
			unsigned int _rtt;
			Node::Type _type;
			unsigned int _port;
			Node::Protocol _protocol;
		};
	}
}

#endif /*IBRDTN_DAEMON_NODE_H_*/
