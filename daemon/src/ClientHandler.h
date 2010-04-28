/*
 * ClientHandler.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#ifndef CLIENTHANDLER_H_
#define CLIENTHANDLER_H_

#include "core/AbstractWorker.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/streams/BundleStreamReader.h"
#include "ibrdtn/streams/BundleFactory.h"
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
		class ClientHandler : public dtn::net::GenericConnection, public dtn::streams::StreamConnection::Callback, public ibrcommon::JoinableThread
		{
		public:
			ClientHandler(ibrcommon::tcpstream *stream);
			virtual ~ClientHandler();

			void iamfree();
			bool free();

			bool isConnected();
			virtual void shutdown();

			virtual void eventShutdown();
			virtual void eventTimeout();
			virtual void eventConnectionUp(const StreamContactHeader &header);

			friend ClientHandler& operator>>(ClientHandler &conn, dtn::data::Bundle &bundle);
			friend ClientHandler& operator<<(ClientHandler &conn, const dtn::data::Bundle &bundle);

			const dtn::data::EID& getPeer() const;

		protected:
			void received(const dtn::streams::StreamContactHeader &h);
			void run();

		private:
			ibrcommon::Mutex _freemutex;
			bool _free;
			bool _running;
			dtn::data::EID _eid;

			auto_ptr<ibrcommon::tcpstream> _stream;
			dtn::streams::StreamConnection _connection;
			StreamContactHeader _contact;
		};
	}
}
#endif /* CLIENTHANDLER_H_ */
