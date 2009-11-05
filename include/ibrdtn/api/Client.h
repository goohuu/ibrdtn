/*
 * ApiClient.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */


#ifndef CLIENT_H_
#define CLIENT_H_

#include "ibrdtn/default.h"
#include "ibrdtn/api/Bundle.h"
#include "ibrdtn/data/Bundle.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/streams/BundleStreamReader.h"
#include "ibrdtn/streams/BundleFactory.h"
#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/utils/Mutex.h"
#include "ibrdtn/utils/MutexLock.h"

using namespace dtn::data;
using namespace dtn::streams;

namespace dtn
{
	namespace api
	{
		class ConnectionException : public dtn::exceptions::Exception
		{
		public:
			ConnectionException(string what = "A connection error occurred.") throw() : Exception(what)
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
		class Client : public StreamConnection
		{
		private:
			class AsyncReceiver : public dtn::utils::JoinableThread
			{
			public:
				AsyncReceiver(Client &client);
				virtual ~AsyncReceiver();

				void run();

			private:
				Client &_client;
				bool _shutdown;
			};


		public:
			/**
			 * Constructor for the API Connection. At least a application suffix
			 * and a existing stream are required. The suffix is appended to the node
			 * id of the daemon. E.g. dtn://node1/example (in this case is "example" the
			 * application id. The stream connects the daemon and this application together
			 * and will be used with the bundle protocol for TCP (draft-irtf-dtnrg-tcp-clayer-02)
			 * provided by the StreamConnection class.
			 */
			Client(string app, iostream &stream, bool async = true);

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

			/**
			 * this method gets called if the connection goes down
			 */
			virtual void shutdown();

		protected:
			/**
			 * This method is called on the receipt of the handshake of the daemon. If
			 * you like to validate your connection you could overload this method, but must
			 * call the super method.
			 */
			virtual void received(dtn::streams::StreamContactHeader &h);
			virtual void received(dtn::api::Bundle &b) {};

		private:
			string _app;
			bool _connected;
			dtn::streams::StreamContactHeader _header;
			Client::AsyncReceiver _receiver;
		};
	}
}

#endif /* CLIENT_H_ */
