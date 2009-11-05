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
#include "ibrdtn/streams/tcpstream.h"
#include "core/EventReceiver.h"

using namespace dtn::streams;

namespace dtn
{
	namespace daemon
	{
		class ClientHandler : public dtn::core::AbstractWorker, public dtn::utils::JoinableThread, public dtn::core::Graveyard::Zombie, public dtn::core::EventReceiver
		{
		public:
			ClientHandler(iostream &stream);
			ClientHandler(int socket);
			virtual ~ClientHandler();

			bool isConnected();
			dtn::net::TransmitReport callbackBundleReceived(const Bundle &b);

			virtual void shutdown();
			void embalm();

			void raiseEvent(const dtn::core::Event *evt);

		protected:
			void received(dtn::streams::StreamContactHeader &h);
			void run();

		private:
			dtn::streams::StreamConnection _connection;
			StreamContactHeader _contact;

			// local eid
			EID _local;

			bool _connected;

			dtn::streams::tcpstream _tcpstream;
		};
	}
}
#endif /* CLIENTHANDLER_H_ */
