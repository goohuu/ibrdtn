/*
 * TCPConvergenceLayer.h
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#ifndef TCPCONVERGENCELAYER_H_
#define TCPCONVERGENCELAYER_H_

#include "Component.h"

#include "core/EventReceiver.h"
#include "core/NodeEvent.h"
#include "core/Node.h"

#include "net/GenericServer.h"
#include "net/ConvergenceLayer.h"
#include "net/DiscoveryService.h"
#include "net/DiscoveryServiceProvider.h"

#include <ibrdtn/data/Bundle.h>
#include <ibrdtn/data/Serializer.h>
#include <ibrdtn/streams/StreamConnection.h>
#include <ibrdtn/streams/StreamContactHeader.h>

#include <ibrcommon/net/tcpserver.h>
#include <ibrcommon/net/tcpstream.h>
#include <ibrcommon/net/NetInterface.h>

#include <memory>
#include <ibrcommon/thread/ThreadSafeQueue.h>

using namespace dtn::streams;

namespace dtn
{
	namespace net
	{
		/**
		 * This class implement a ConvergenceLayer for TCP/IP.
		 * http://tools.ietf.org/html/draft-irtf-dtnrg-tcp-clayer-02
		 */
		class TCPConvergenceLayer : public dtn::daemon::Component, public ConvergenceLayer, public DiscoveryServiceProvider
		{
		public:
			class TCPConnection : public GenericConnection, public StreamConnection::Callback
			{
			public:
				TCPConnection(ibrcommon::tcpstream *stream);
				virtual ~TCPConnection();

				/**
				 * This method sets this instance free. (Mark it for the "garbage collector".)
				 */
				void iamfree();

				/**
				 * initialize this connection by send and receive
				 * the connection header.
				 * @param header
				 */
				void initialize(const dtn::data::EID &name, const size_t timeout = 10);

				/**
				 * shutdown the whole tcp connection
				 */
				void shutdown();

				/**
				 * Get the header of this connection
				 * @return
				 */
				const StreamContactHeader getHeader() const;

				/**
				 * Get the associated Node object
				 * @return
				 */
				const dtn::core::Node& getNode() const;

				/**
				 * callback methods for tcpstream
				 */
				virtual void eventShutdown();
				virtual void eventTimeout();
				virtual void eventError();
				virtual void eventConnectionUp(const StreamContactHeader &header);
				virtual void eventConnectionDown();

				virtual void eventBundleRefused();
				virtual void eventBundleForwarded();
				virtual void eventBundleAck(size_t ack);

				bool free();

				dtn::core::Node::Protocol getDiscoveryProtocol() const;

				/**
				 * queue a bundle for this connection
				 * @param bundle
				 */
				void queue(const dtn::data::Bundle &bundle);

				friend TCPConvergenceLayer::TCPConnection& operator>>(TCPConvergenceLayer::TCPConnection &conn, dtn::data::Bundle &bundle);
				friend TCPConvergenceLayer::TCPConnection& operator<<(TCPConvergenceLayer::TCPConnection &conn, const dtn::data::Bundle &bundle);

			protected:
				void handshake();

				void rejectTransmission();

			private:
				/**
				 * Receiver sub-process
				 * This sub-process is implemented as a thread
				 * and do a constant read to this connection until
				 * the receiver is stopped or a exception is thrown.
				 */
				class Receiver : public ibrcommon::JoinableThread
				{
				public:
					Receiver(TCPConnection &connection);
					virtual ~Receiver();
					void run();
					void shutdown();
					void finally();

				private:
					TCPConnection &_connection;
				};

				class Sender : public ibrcommon::JoinableThread, public ibrcommon::ThreadSafeQueue<dtn::data::Bundle>
				{
				public:
					Sender(TCPConnection &connection);
					virtual ~Sender();
					void run();
					void shutdown();
					void finally();

				private:
					TCPConnection &_connection;
				};

				ibrcommon::Mutex _freemutex;
				bool _free;

				StreamContactHeader _peer;
				dtn::core::Node _node;

				std::auto_ptr<ibrcommon::tcpstream> _tcpstream;
				StreamConnection _stream;

				// This thread gets awaiting bundles of the queue
				// and transmit them to the peer.
				Sender _sender;

				// This thread receive all bundles
				Receiver _receiver;

				// handshake variables
				dtn::data::EID _name;
				size_t _timeout;

				ibrcommon::ThreadSafeQueue<dtn::data::Bundle> _sentqueue;
				size_t _lastack;

				bool _shutdown;
			};

			class Server : public dtn::net::GenericServer<TCPConvergenceLayer::TCPConnection>, public dtn::core::EventReceiver
			{
			public:
				Server(ibrcommon::NetInterface net, int port);
				virtual ~Server();

				/**
				 * Queue a new transmission job for this convergence layer.
				 * @param job
				 */
				void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

				/**
				 * handler for events
				 */
				void raiseEvent(const dtn::core::Event *evt);

			protected:
				TCPConvergenceLayer::TCPConnection* accept();
				void listen();
				void shutdown();

				void connectionUp(TCPConvergenceLayer::TCPConnection *conn);
				void connectionDown(TCPConvergenceLayer::TCPConnection *conn);

			private:
				class Connection
				{
				public:
					Connection(TCPConvergenceLayer::TCPConnection *conn, const dtn::core::Node &node, const bool &active = false);
					~Connection();

					bool match(const dtn::data::EID &destination) const;
					bool match(const dtn::core::NodeEvent &evt) const;

					TCPConvergenceLayer::TCPConnection& operator*();

					TCPConvergenceLayer::TCPConnection *_connection;
					dtn::core::Node _peer;
					bool _active;
				};

				TCPConnection* getConnection(const dtn::core::Node &n);
				ibrcommon::tcpserver _tcpsrv;
				ibrcommon::Conditional _connection_lock;
				std::list<Connection> _connections;
			};

			/**
			 * Constructor
			 * @param[in] bind_addr The address to bind.
			 * @param[in] port The port to use.
			 */
			TCPConvergenceLayer(ibrcommon::NetInterface net, int port);

			/**
			 * Destructor
			 */
			virtual ~TCPConvergenceLayer();

			dtn::core::Node::Protocol getDiscoveryProtocol() const;

			void initialize();
			void startup();
			void terminate();

			/**
			 * this method updates the given values
			 */
			void update(std::string &name, std::string &data);
			bool onInterface(const ibrcommon::NetInterface &net) const;

			/**
			 * Queue a new transmission job for this convergence layer.
			 * @param job
			 */
			void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

		private:
			static const int DEFAULT_PORT;
			bool _running;

			ibrcommon::NetInterface _net;
			int _port;
			TCPConvergenceLayer::Server _server;
		};
	}
}

#endif /* TCPCONVERGENCELAYER_H_ */
