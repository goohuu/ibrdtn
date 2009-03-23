#ifndef NODE_H_
#define NODE_H_

#include <string>

using namespace std;

namespace dtn
{
	namespace core
	{
		class ConvergenceLayer;

		/**
		 * Specify a node type.
		 * FLOATING is a node, if it's not statically reachable.
		 * PERMANENT is used for static nodes with are permanently reachable.
		 */
		enum NodeType
		{
			FLOATING = 0,
			PERMANENT = 1
		};

		/**
		 * A Node instance holds all informations of a neighboring node.
		 */
		class Node
		{
		public:
			/**
			 * constructor
			 * @param type set the node type
			 * @sa NoteType
			 */
			Node(NodeType type = PERMANENT);

			/**
			 * copy the object
			 */
			Node(const Node &k);

			/**
			 * destructor
			 */
			virtual ~Node();

			/**
			 * get the node type.
			 * @return The type of the node.
			 * @sa NoteType
			 */
			NodeType getType() const;

			/**
			 * Set the address of the node.
			 * @param address The address of the node. (e.g. 10.0.0.1 for IP)
			 */
			void setAddress(string address);

			/**
			 * Get the address of the node.
			 * @return The address of the node. (e.g. 10.0.0.1 for IP)
			 */
			string getAddress() const;

			void setPort(unsigned int port);
			unsigned int getPort() const;

			/*
			 * Set a description for the node.
			 * @param description a description
			 */
			void setDescription(string description);

			/**
			 * Get the assigned description for the node.
			 * @return The description.
			 */
			string getDescription() const;

			/**
			 * Set the URI (EID) of the node.
			 * @param uri The URI of the node.
			 */
			void setURI(string uri);

			/**
			 * Get the URI (EID) of the node.
			 * @return The URI of the node.
			 */
			string getURI() const;

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
			 bool equals(Node &node);

			 ConvergenceLayer* getConvergenceLayer() const;
			 void setConvergenceLayer(ConvergenceLayer *cl);

			 void setPosition(pair<double,double> value);
			 pair<double,double> getPosition() const;

		private:
			string m_address;
			string m_description;
			string m_uri;
			int m_timeout;
			NodeType m_type;
			unsigned int m_port;
			ConvergenceLayer *m_cl;

			pair<double,double> m_position;
		};
	}
}

#endif /*NODE_H_*/
