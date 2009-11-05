/*
 * TCPConvergenceLayer.h
 *
 *  Created on: 05.08.2009
 *      Author: morgenro
 */

#ifndef TCPCONVERGENCELAYER_H_
#define TCPCONVERGENCELAYER_H_

#include "ibrdtn/default.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/utils/tcpserver.h"
#include "ibrdtn/streams/tcpstream.h"
#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/streams/StreamContactHeader.h"

#include "core/Graveyard.h"
#include "core/EventReceiver.h"
#include "core/Node.h"

#include "net/ConvergenceLayer.h"
#include "net/BundleConnection.h"

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
		 : public dtn::utils::tcpserver, public dtn::utils::JoinableThread, public ConvergenceLayer
		{
		public:
			class TCPConnection : public dtn::streams::StreamConnection, public BundleConnection, public dtn::core::Graveyard::Zombie, public dtn::core::EventReceiver
			{
			public:
				TCPConnection(TCPConvergenceLayer &cl, int socket, tcpstream::stream_direction d, size_t timeout = 30);
				virtual ~TCPConnection();

				const dtn::core::Node& getNode() const;

				void read(dtn::data::Bundle &bundle);
				void write(const dtn::data::Bundle &bundle);

				void initialize(dtn::streams::StreamContactHeader header);
				virtual void shutdown();

				void raiseEvent(const dtn::core::Event *evt);

				const StreamContactHeader getHeader() const;

				// method to prepare the class for deletion
				void embalm();

				bool isConnected();

			private:
				TCPConvergenceLayer &_cl;
				dtn::streams::tcpstream _stream;
				dtn::core::Node _node;

				dtn::streams::StreamContactHeader _out_header;
				dtn::streams::StreamContactHeader _in_header;
				bool _connected;
			};

			/**
			 * Constructor
			 * @param[in] bind_addr The address to bind.
			 * @param[in] port The port to use.
			 */
			TCPConvergenceLayer(string bind_addr = "0.0.0.0", unsigned short port = 4556);

			/**
			 * Destructor
			 */
			virtual ~TCPConvergenceLayer();

		protected:
			virtual void run();
			void add(TCPConnection *conn);
			void remove(TCPConnection *conn);

		private:
			static const int DEFAULT_PORT;
			bool _running;

			TCPConnection* openConnection(const dtn::core::Node &n);
			BundleConnection* getConnection(const dtn::core::Node &n);

			std::list<TCPConnection*> _connections;

			dtn::streams::StreamContactHeader _header;
			dtn::utils::Mutex _connection_lock;
		};
	}
}

#endif /* TCPCONVERGENCELAYER_H_ */
