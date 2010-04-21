/*
 * TCPConvergenceLayer.h
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#ifndef TCPCONVERGENCELAYER_H_
#define TCPCONVERGENCELAYER_H_

#include "Component.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrcommon/net/tcpserver.h"
#include "ibrcommon/net/tcpstream.h"
#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/streams/StreamContactHeader.h"

#include "core/Graveyard.h"
#include "core/EventReceiver.h"
#include "core/Node.h"

#include "net/ConvergenceLayer.h"
#include "net/BundleConnection.h"
#include "net/DiscoveryService.h"
#include "net/DiscoveryServiceProvider.h"
#include "ibrcommon/net/NetInterface.h"

#include <memory>

namespace dtn
{
	namespace net
	{
		class ConnectionManager;

		/**
		 * This class implement a ConvergenceLayer for TCP/IP.
		 * http://tools.ietf.org/html/draft-irtf-dtnrg-tcp-clayer-02
		 */
		class TCPConvergenceLayer
		 : public ibrcommon::tcpserver, public dtn::daemon::IndependentComponent, public ConvergenceLayer, public DiscoveryServiceProvider
		{
		public:
			class TCPConnection : public BundleConnection, public dtn::core::Graveyard::Zombie, public dtn::core::EventReceiver, public StreamConnection::Callback
			{
			public:
				TCPConnection(TCPConvergenceLayer &cl, ibrcommon::tcpstream *stream);
				virtual ~TCPConnection();

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
				 * Read a bundle from this connection
				 * @param bundle
				 */
				void read(dtn::data::Bundle &bundle);

				/**
				 * Write a bundle to this connection
				 * @param bundle
				 */
				void write(const dtn::data::Bundle &bundle);

				/**
				 * method to prepare the class for deletion
				 */
				void embalm();

				bool isConnected();
				bool isBusy() const;

				/**
				 * callback methods for tcpstream
				 */
				virtual void eventShutdown();
				virtual void eventTimeout();
				virtual void eventConnectionUp(const StreamContactHeader &header);

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

				std::auto_ptr<ibrcommon::tcpstream> _tcpstream;
				StreamConnection _stream;

				bool _busy;
				ibrcommon::Mutex _busymutex;

				TCPConvergenceLayer &_cl;

				// This thread receives incoming bundles and forward them
				// to the storage.
				Receiver _receiver;

				StreamContactHeader _peer;
				dtn::core::Node _node;
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

			/**
			 * this method updates the given values
			 */
			void update(std::string &name, std::string &data);

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();
			void add(TCPConnection *conn);
			void remove(TCPConnection *conn);

		private:
			ibrcommon::NetInterface _net;
			static const int DEFAULT_PORT;
			bool _running;

			BundleConnection* getConnection(const dtn::core::Node &n);

			std::list<TCPConnection*> _connections;
			ibrcommon::Conditional _connection_lock;
		};
	}
}

#endif /* TCPCONVERGENCELAYER_H_ */
