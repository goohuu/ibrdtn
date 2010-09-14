/*
 * ClientHandler.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "ClientHandler.h"
#include "core/GlobalEvent.h"
#include "core/BundleCore.h"
#include "net/BundleReceivedEvent.h"
#include "core/BundleEvent.h"
#include <ibrdtn/streams/StreamContactHeader.h>
#include <ibrdtn/data/Serializer.h>
#include <iostream>

using namespace dtn::data;
using namespace dtn::streams;
using namespace dtn::core;

namespace dtn
{
	namespace daemon
	{
		ClientHandler::ClientHandler(ibrcommon::tcpstream *stream)
		 : ibrcommon::JoinableThread(), _free(false), _running(true), _stream(stream), _connection(*this, *_stream)
		{
		}

		ClientHandler::~ClientHandler()
		{
			shutdown();
			join();
		}

		const dtn::data::EID& ClientHandler::getPeer() const
		{
			return _eid;
		}

		void ClientHandler::iamfree()
		{
			_free = true;
		}

		bool ClientHandler::free()
		{
			ibrcommon::MutexLock l(_freemutex);
			return _free;
		}

		void ClientHandler::eventShutdown()
		{
			// shutdown message received
			try {
				//(*_stream).done();
				(*_stream).close();
			} catch (ibrcommon::ConnectionClosedException ex) {

			}
		}

		void ClientHandler::eventTimeout()
		{
			//(*_stream).done();
			(*_stream).close();
		}

		void ClientHandler::eventConnectionUp(const StreamContactHeader &header)
		{
			if (BundleCore::local.getScheme() == dtn::data::EID::CBHE_SCHEME)
			{
				// This node is working with compressed addresses
				// generate a number
				_eid = BundleCore::local + "." + header._localeid.getNode();
			}
			else
			{
				// contact received event
				_eid = BundleCore::local + "/" + header._localeid.getNode();
			}
		}

		void ClientHandler::eventError()
		{
			(*_stream).close();
		}

		void ClientHandler::eventConnectionDown()
		{
			try {
				while (true)
				{
					const dtn::data::Bundle bundle = _sentqueue.frontpop();

//					if (_lastack > 0)
//					{
//						// some data are already acknowledged, make a fragment?
//						//TODO: make a fragment
//						TransferAbortedEvent::raise(EID(_node.getURI()), bundle);
//					}
//					else
//					{
//						// raise transfer abort event for all bundles without an ACK
//						TransferAbortedEvent::raise(EID(_node.getURI()), bundle);
//					}

					// set last ack to zero
					_lastack = 0;
				}
			} catch (ibrcommon::Exception ex) {
				// queue emtpy
			}

			ibrcommon::MutexLock l(_freemutex);
			iamfree();
		}

		void ClientHandler::eventBundleRefused()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.frontpop();

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::Exception ex) {
				// pop on empty queue!
			}
		}

		void ClientHandler::eventBundleForwarded()
		{
			try {
				const dtn::data::Bundle bundle = _sentqueue.frontpop();

				// raise bundle event
				dtn::core::BundleEvent::raise(bundle, BUNDLE_DELIVERED);

				// set ACK to zero
				_lastack = 0;

			} catch (ibrcommon::Exception ex) {
				// pop on empty queue!
			}
		}

		void ClientHandler::eventBundleAck(size_t ack)
		{
			_lastack = ack;
		}

		bool ClientHandler::isConnected()
		{
			return _connection.isConnected();
		}

		void ClientHandler::shutdown()
		{
			// shutdown the connection
			_connection.shutdown();

			// close the reading thread
			JoinableThread::stop();
		}

		void ClientHandler::finally()
		{
			eventShutdown();
			iamfree();
		}

		void ClientHandler::run()
		{
			try {
				try {
					char flags = 0;

					// request acknowledgements
					flags |= dtn::streams::StreamContactHeader::REQUEST_ACKNOWLEDGMENTS;

					// do the handshake
					_connection.handshake(dtn::core::BundleCore::local, 10, flags);

					BundleStorage &storage = BundleCore::getInstance().getStorage();

					while (_running)
					{
						try {
							dtn::data::Bundle b = storage.get( getPeer() );
							dtn::data::DefaultSerializer(_connection) << b; _connection << std::flush;
							storage.remove(b);
						} catch (dtn::core::BundleStorage::NoBundleFoundException ex) {
							break;
						}
					}

					while (_running)
					{
						dtn::data::Bundle bundle;
						dtn::data::DefaultDeserializer(_connection) >> bundle;

						// create a new sequence number
						bundle.relabel();

						// check address fields for "api:me", this has to be replaced
						dtn::data::EID clienteid("api:me");

						// set the source address to the sending EID
						bundle._source = _eid;

						if (bundle._destination == clienteid) bundle._destination = _eid;
						if (bundle._reportto == clienteid) bundle._reportto = _eid;
						if (bundle._custodian == clienteid) bundle._custodian = _eid;

						// raise default bundle received event
						dtn::net::BundleReceivedEvent::raise(EID(), bundle);

						yield();
					}

					_connection.shutdown();
				} catch (ibrcommon::IOException ex) {
					_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
					_running = false;
				} catch (dtn::InvalidDataException ex) {
					_connection.shutdown(StreamConnection::CONNECTION_SHUTDOWN_ERROR);
					_running = false;
				}
			} catch (std::exception) {
			}
		}

		ClientHandler& operator>>(ClientHandler &conn, dtn::data::Bundle &bundle)
		{
			// get a bundle
			dtn::data::DefaultDeserializer(conn._connection, dtn::core::BundleCore::getInstance()) >> bundle;

			return conn;
		}

		ClientHandler& operator<<(ClientHandler &conn, const dtn::data::Bundle &bundle)
		{
			// add bundle to the queue
			conn._sentqueue.push(bundle);

			// transmit the bundle
			dtn::data::DefaultSerializer(conn._connection) << bundle;

			// mark the end of the bundle
			conn._connection << std::flush;

			return conn;
		}
	}
}

