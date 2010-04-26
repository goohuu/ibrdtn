/*
 * TCPConvergenceLayer.h
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#ifndef TCPCONVERGENCELAYER_H_
#define TCPCONVERGENCELAYER_H_

#include "Component.h"
#include "net/GenericServer.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrcommon/net/tcpserver.h"
#include "ibrcommon/net/tcpstream.h"
#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/streams/StreamContactHeader.h"

#include "core/EventReceiver.h"
#include "core/Node.h"

#include "net/ConvergenceLayer.h"
#include "net/DiscoveryService.h"
#include "net/DiscoveryServiceProvider.h"
#include "ibrcommon/net/NetInterface.h"

#include <memory>
#include <queue>

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
			class TCPConnection : public GenericConnection, public dtn::core::EventReceiver, public StreamConnection::Callback
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
				 * handler for events
				 */
				void raiseEvent(const dtn::core::Event *evt);

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
				virtual void eventConnectionUp(const StreamContactHeader &header);

				bool free();

				const dtn::core::NodeProtocol getDiscoveryProtocol() const;

				/**
				 * queue a bundle for this connection
				 * @param bundle
				 */
				void queue(const dtn::data::Bundle &bundle);

				friend TCPConvergenceLayer::TCPConnection& operator>>(TCPConvergenceLayer::TCPConnection &conn, dtn::data::Bundle &bundle);
				friend TCPConvergenceLayer::TCPConnection& operator<<(TCPConvergenceLayer::TCPConnection &conn, const dtn::data::Bundle &bundle);

			protected:
				ibrcommon::Conditional _queue_lock;
				std::queue<dtn::data::Bundle> _bundles;

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

				private:
					bool _running;
					TCPConnection &_connection;
				};

				class Sender : public ibrcommon::JoinableThread
				{
				public:
					Sender(TCPConnection &connection);
					virtual ~Sender();
					void run();
					void shutdown();

				private:
					bool _running;
					TCPConnection &_connection;
				};

				ibrcommon::Mutex _freemutex;
				bool _free;

				StreamContactHeader _peer;
				dtn::core::Node _node;

				std::auto_ptr<ibrcommon::tcpstream> _tcpstream;
				StreamConnection _stream;

				// This thread receives incoming bundles and forward them
				// to the storage.
				Receiver _receiver;

				// This thread gets awaiting bundles of the queue
				// and transmit them to the peer.
				Sender _sender;
			};

			class Server : public dtn::net::GenericServer<TCPConvergenceLayer::TCPConnection>
			{
			public:
				Server(ibrcommon::NetInterface net);
				virtual ~Server();

				/**
				 * Queue a new transmission job for this convergence layer.
				 * @param job
				 */
				void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

			protected:
				TCPConvergenceLayer::TCPConnection* accept();
				void listen();
				void shutdown();

				void connectionUp(TCPConvergenceLayer::TCPConnection *conn);
				void connectionDown(TCPConvergenceLayer::TCPConnection *conn);

			private:
				TCPConnection* getConnection(const dtn::core::Node &n);
				ibrcommon::tcpserver _tcpsrv;
				std::list<TCPConnection*> _connections;
				ibrcommon::Conditional _connection_lock;
			};

			/**
			 * Constructor
			 * @param[in] bind_addr The address to bind.
			 * @param[in] port The port to use.
			 */
			TCPConvergenceLayer(ibrcommon::NetInterface net);

			/**
			 * Destructor
			 */
			virtual ~TCPConvergenceLayer();

			const dtn::core::NodeProtocol getDiscoveryProtocol() const;

			void initialize();
			void startup();
			void terminate();

			/**
			 * this method updates the given values
			 */
			void update(std::string &name, std::string &data);

			/**
			 * Queue a new transmission job for this convergence layer.
			 * @param job
			 */
			void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

		private:
			static const int DEFAULT_PORT;
			bool _running;

			ibrcommon::NetInterface _net;
			TCPConvergenceLayer::Server _server;
		};
	}
}

#endif /* TCPCONVERGENCELAYER_H_ */
