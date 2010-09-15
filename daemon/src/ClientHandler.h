/*
 * ClientHandler.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#ifndef CLIENTHANDLER_H_
#define CLIENTHANDLER_H_

#include "core/AbstractWorker.h"
#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "ibrcommon/net/tcpstream.h"
#include "core/EventReceiver.h"
#include "net/GenericServer.h"
#include <memory>

using namespace dtn::streams;

namespace dtn
{
	namespace daemon
	{
		class ClientHandler : public dtn::net::GenericConnection<ClientHandler>, public dtn::streams::StreamConnection::Callback, public ibrcommon::JoinableThread
		{
		public:
			ClientHandler(dtn::net::GenericServer<ClientHandler> &srv, ibrcommon::tcpstream *stream);
			virtual ~ClientHandler();

			bool isConnected();
			virtual void shutdown();

			virtual void eventShutdown();
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

		protected:
			void received(const dtn::streams::StreamContactHeader &h);
			void run();
			void finally();

		private:
			bool _running;
			dtn::data::EID _eid;

			auto_ptr<ibrcommon::tcpstream> _stream;
			dtn::streams::StreamConnection _connection;
			StreamContactHeader _contact;

			ibrcommon::ThreadSafeQueue<dtn::data::Bundle> _sentqueue;
			size_t _lastack;
		};
	}
}
#endif /* CLIENTHANDLER_H_ */
