/*
 * ApiClient.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */


#ifndef CLIENT_H_
#define CLIENT_H_

#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/streams/StreamConnection.h"
#include "ibrcommon/net/tcpstream.h"
#include "ibrcommon/thread/Mutex.h"
#include "ibrcommon/thread/MutexLock.h"
#include "ibrcommon/Exceptions.h"
#include "ibrcommon/thread/Queue.h"

using namespace dtn::data;
using namespace dtn::streams;

namespace dtn
{
	namespace api
	{
		class ConnectionException : public ibrcommon::Exception
		{
		public:
			ConnectionException(string what = "A connection error occurred.") throw() : ibrcommon::Exception(what)
			{
			};
		};

		/**
		 * This is an abstract class is the base for any API connection to a
		 * IBR-DTN daemon. It uses a existing stream to communicate bidirectional
		 * with the daemon.
		 *
		 * For asynchronous reception of bundle this class implements a thread.
		 */
		class Client : public StreamConnection, public StreamConnection::Callback
		{
		private:
			class AsyncReceiver : public ibrcommon::JoinableThread
			{
			public:
				AsyncReceiver(Client &client);
				virtual ~AsyncReceiver();

			protected:
				void run();
				bool __cancellation();

			private:
				Client &_client;
			};

			enum HANDSHAKE_FLAGS
			{
				HANDSHAKE_SENDONLY = 0x80
			};


		public:
			enum COMMUNICATION_MODE
			{
				MODE_BIDIRECTIONAL = 0, 	// bidirectional
				MODE_SENDONLY = 1 			// unidirectional, no reception of bundles
			};

			/**
			 * Constructor for the API Connection. At least a application suffix
			 * and a existing stream are required. The suffix is appended to the node
			 * id of the daemon. E.g. dtn://node1/example (in this case is "example" the
			 * application id. The stream connects the daemon and this application together
			 * and will be used with the bundle protocol for TCP (draft-irtf-dtnrg-tcp-clayer-02)
			 * provided by the StreamConnection class.
			 */
			Client(COMMUNICATION_MODE mode, string app, ibrcommon::tcpstream &stream);
			Client(string app, ibrcommon::tcpstream &stream);

			/**
			 * Virtual destructor for this class.
			 */
			virtual ~Client();

			/**
			 * This method starts the thread and execute the handshake with the server.
			 */
			void connect();

			/**
			 * @return True, if the Client is connected to the daemon.
			 */
			bool isConnected();

			void close();

			/**
			 * this method gets called if the connection goes down
			 */
			virtual void eventShutdown();
			virtual void eventTimeout();
			virtual void eventError();
			virtual void eventConnectionUp(const StreamContactHeader &header);
			virtual void eventConnectionDown();
			virtual void eventBundleRefused();
			virtual void eventBundleForwarded();
			virtual void eventBundleAck(size_t ack);

			dtn::api::Bundle getBundle(size_t timeout = 0);

			size_t lastack;

		protected:
			/**
			 * This method is called on the receipt of the handshake of the daemon. If
			 * you like to validate your connection you could overload this method, but must
			 * call the super method.
			 */
			virtual void received(const dtn::streams::StreamContactHeader &h);
			virtual void received(const dtn::api::Bundle &b);

		private:
			ibrcommon::tcpstream &_stream;
			COMMUNICATION_MODE _mode;
			string _app;
			bool _connected;
			bool _async;
			dtn::streams::StreamContactHeader _header;
			Client::AsyncReceiver _receiver;

			ibrcommon::Queue<dtn::api::Bundle> _inqueue;
		};
	}
}

#endif /* CLIENT_H_ */
