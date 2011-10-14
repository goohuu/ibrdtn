/*
 * EventConnection.cpp
 *
 *  Created on: 13.10.2011
 *      Author: morgenro
 */

#include "EventConnection.h"

#include "core/NodeEvent.h"
#include "core/GlobalEvent.h"
#include "core/CustodyEvent.h"
#include "routing/QueueBundleEvent.h"
#include "net/TransferAbortedEvent.h"
#include "net/TransferCompletedEvent.h"
#include "net/ConnectionEvent.h"

#include <ibrdtn/utils/Utils.h>

namespace dtn
{
	namespace api
	{
		EventConnection::EventConnection(ClientHandler &client, ibrcommon::tcpstream &stream)
		 : ProtocolHandler(client, stream), _running(true)
		{
		}

		EventConnection::~EventConnection()
		{
		}

		void EventConnection::raiseEvent(const dtn::core::Event *evt)
		{
			ibrcommon::MutexLock l(_mutex);
			if (!_running) return;

			try {
				const dtn::core::NodeEvent &node = dynamic_cast<const dtn::core::NodeEvent&>(*evt);

				// ignore NODE_INFO_UPDATED
				if (node.getAction() == dtn::core::NODE_INFO_UPDATED) return;

				// start with the event tag
				_stream << "<event name=\"" << node.getName() << "\" action=\"";

				switch (node.getAction())
				{
				case dtn::core::NODE_AVAILABLE:
					_stream << "available";
					break;
				case dtn::core::NODE_UNAVAILABLE:
					_stream << "unavailable";
					break;
				default:
					break;
				}

				_stream << "\">";

				// write the node eid
				_stream << "<eid>" << node.getNode().getEID().getString() << "</eid>";

				// close the event
				_stream << "</event>" << std::flush;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::GlobalEvent &global = dynamic_cast<const dtn::core::GlobalEvent&>(*evt);

				// start with the event tag
				_stream << "<event name=\"" << global.getName() << "\" action=\"";

				switch (global.getAction())
				{
				case dtn::core::GlobalEvent::GLOBAL_BUSY:
					_stream << "busy";
					break;
				case dtn::core::GlobalEvent::GLOBAL_IDLE:
					_stream << "idle";
					break;
				case dtn::core::GlobalEvent::GLOBAL_POWERSAVE:
					_stream << "powersave";
					break;
				case dtn::core::GlobalEvent::GLOBAL_RELOAD:
					_stream << "reload";
					break;
				case dtn::core::GlobalEvent::GLOBAL_SHUTDOWN:
					_stream << "shutdown";
					break;
				case dtn::core::GlobalEvent::GLOBAL_SUSPEND:
					_stream << "suspend";
					break;
				default:
					break;
				}

				// close the event
				_stream << "\" />" << std::flush;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::CustodyEvent &custody = dynamic_cast<const dtn::core::CustodyEvent&>(*evt);

			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::TransferAbortedEvent &aborted = dynamic_cast<const dtn::net::TransferAbortedEvent&>(*evt);

			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::TransferCompletedEvent &completed = dynamic_cast<const dtn::net::TransferCompletedEvent&>(*evt);

			} catch (const std::bad_cast&) { };

			try {
				const dtn::net::ConnectionEvent &connection = dynamic_cast<const dtn::net::ConnectionEvent&>(*evt);

				// start with the event tag
				_stream << "<event name=\"" << connection.getName() << "\" action=\"";

				switch (connection.state)
				{
				case dtn::net::ConnectionEvent::CONNECTION_UP:
					_stream << "up";
					break;
				case dtn::net::ConnectionEvent::CONNECTION_DOWN:
					_stream << "down";
					break;
				case dtn::net::ConnectionEvent::CONNECTION_SETUP:
					_stream << "setup";
					break;
				case dtn::net::ConnectionEvent::CONNECTION_TIMEOUT:
					_stream << "timeout";
					break;
				default:
					break;
				}

				_stream << "\">";

				// write the peer eid
				_stream << "<peer>" << connection.peer.getString() << "</peer>";

				// close the event
				_stream << "</event>" << std::flush;
			} catch (const std::bad_cast&) { };

			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);

				// start with the event tag
				_stream << "<event name=\"" << queued.getName() << "\">";

				// write the bundle data
				_stream << "<source>" << queued.bundle.source.getString() << "</source>";
				_stream << "<timestamp>" << queued.bundle.timestamp << "</timestamp>";
				_stream << "<sequencenumber>" << queued.bundle.sequencenumber << "</sequencenumber>";
				_stream << "<lifetime>" << queued.bundle.lifetime << "</lifetime>";
				_stream << "<procflags>" << queued.bundle.procflags << "</procflags>";

				// write the destination eid
				_stream << "<destination>" << queued.bundle.destination.getString() << "</destination>";

				// close the event
				_stream << "</event>" << std::flush;
			} catch (const std::bad_cast&) { };
		}

		void EventConnection::run()
		{
			std::string buffer;

			// run as long the stream is ok
			while (_stream.good())
			{
				getline(_stream, buffer);

				// search for '\r\n' and remove the '\r'
				std::string::reverse_iterator iter = buffer.rbegin();
				if ( (*iter) == '\r' ) buffer = buffer.substr(0, buffer.length() - 1);

				std::vector<std::string> cmd = dtn::utils::Utils::tokenize(" ", buffer);
				if (cmd.size() == 0) continue;

				// return to previous level
				if (cmd[0] == "exit") break;
			}

			ibrcommon::MutexLock l(_mutex);
			_running = false;
		}

		void EventConnection::setup()
		{
			// bind to several events
			bindEvent(dtn::core::NodeEvent::className);
			bindEvent(dtn::core::GlobalEvent::className);
			bindEvent(dtn::core::CustodyEvent::className);
			bindEvent(dtn::net::TransferAbortedEvent::className);
			bindEvent(dtn::net::TransferCompletedEvent::className);
			bindEvent(dtn::net::ConnectionEvent::className);
			bindEvent(dtn::routing::QueueBundleEvent::className);

			// send xml header
			_stream << "<?xml version='1.0' encoding='UTF-8' ?>" << "<stream:stream xmlns='ibrdtn:api:event'>" << std::flush;
		}

		void EventConnection::finally()
		{
			// send xml closer
			_stream << "</stream>" << std::endl;

			// unbind to events
			unbindEvent(dtn::core::NodeEvent::className);
			unbindEvent(dtn::core::GlobalEvent::className);
			unbindEvent(dtn::core::CustodyEvent::className);
			unbindEvent(dtn::net::TransferAbortedEvent::className);
			unbindEvent(dtn::net::TransferCompletedEvent::className);
			unbindEvent(dtn::net::ConnectionEvent::className);
			unbindEvent(dtn::routing::QueueBundleEvent::className);
		}

		bool EventConnection::__cancellation()
		{
			return true;
		}
	} /* namespace api */
} /* namespace dtn */
