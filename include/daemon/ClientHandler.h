/*
 * ClientHandler.h
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#ifndef CLIENTHANDLER_H_
#define CLIENTHANDLER_H_

#include "ibrdtn/default.h"
#include "core/AbstractWorker.h"
#include "ibrdtn/streams/BundleStreamWriter.h"
#include "ibrdtn/streams/BundleStreamReader.h"
#include "ibrdtn/streams/BundleFactory.h"
#include "ibrdtn/streams/StreamConnection.h"
#include "ibrdtn/streams/StreamContactHeader.h"
#include "core/Graveyard.h"
#include "ibrcommon/net/tcpstream.h"
#include "core/EventReceiver.h"
#include <memory>

using namespace dtn::streams;

namespace dtn
{
	namespace daemon
	{
		class ClientHandler : public dtn::core::AbstractWorker, public ibrcommon::JoinableThread, public dtn::core::Graveyard::Zombie, public dtn::core::EventReceiver, public dtn::streams::StreamConnection::Callback
		{
		public:
			ClientHandler(ibrcommon::tcpstream *stream);
			virtual ~ClientHandler();

			bool isConnected();
			void callbackBundleReceived(const Bundle &b);

			virtual void shutdown();
			void embalm();

			void raiseEvent(const dtn::core::Event *evt);

			virtual void eventShutdown();
			virtual void eventTimeout();
			virtual void eventConnectionUp(const StreamContactHeader &header);

		protected:
			void received(const dtn::streams::StreamContactHeader &h);
			void run();

		private:
			auto_ptr<ibrcommon::tcpstream> _stream;
			dtn::streams::StreamConnection _connection;
			StreamContactHeader _contact;

			bool _connected;
		};
	}
}
#endif /* CLIENTHANDLER_H_ */
