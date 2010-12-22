/*
 * ClientHandler.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#ifndef CLIENTHANDLER_H_
#define CLIENTHANDLER_H_

#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrcommon/net/tcpstream.h"
#include "core/EventReceiver.h"
#include <memory>

using namespace dtn::streams;

namespace dtn
{
	namespace daemon
	{
		class ClientHandler;

		class ApiServerInterface
		{
		public:
			virtual void connectionUp(ClientHandler *conn) = 0;
			virtual void connectionDown(ClientHandler *conn) = 0;
		};

		class ClientHandler : public dtn::streams::StreamConnection::Callback, public ibrcommon::DetachedThread
		{
		public:
			ClientHandler(ApiServerInterface &srv, ibrcommon::tcpstream *stream, size_t connectionid);
			virtual ~ClientHandler();

			size_t id;

			virtual void initialize();
			virtual void shutdown();

			virtual void eventShutdown(StreamConnection::ConnectionShutdownCases csc);
			virtual void eventTimeout();
			virtual void eventError();
			virtual void eventConnectionDown();
			virtual void eventConnectionUp(const StreamContactHeader &header);

			virtual void eventBundleRefused();
			virtual void eventBundleForwarded();
			virtual void eventBundleAck(size_t ack);

			friend ClientHandler& operator>>(ClientHandler &conn, dtn::data::Bundle &bundle);
			friend ClientHandler& operator<<(ClientHandler &conn, const dtn::data::Bundle &bundle);

			const dtn::data::EID& getPeer() const;

			void queue(const dtn::data::Bundle &bundle);

		protected:
			void received(const dtn::streams::StreamContactHeader &h);
			void run();
			void finally();

		private:
			class Sender : public ibrcommon::JoinableThread, public ibrcommon::Queue<dtn::data::Bundle>
			{
			public:
				Sender(ClientHandler &client);
				virtual ~Sender();

			protected:
				void run();
				bool __cancellation();

			private:
				ClientHandler &_client;
			};

			ApiServerInterface &_srv;
			ClientHandler::Sender _sender;

			dtn::data::EID _eid;

			auto_ptr<ibrcommon::tcpstream> _stream;
			dtn::streams::StreamConnection _connection;
			StreamContactHeader _contact;

			ibrcommon::Queue<dtn::data::Bundle> _sentqueue;
			size_t _lastack;
		};
	}
}
#endif /* CLIENTHANDLER_H_ */
