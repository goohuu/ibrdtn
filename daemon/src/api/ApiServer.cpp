/*
 * ApiServer.cpp
 *
 *  Created on: 24.06.2009
 *      Author: morgenro
 */

#include "config.h"
#include "Configuration.h"
#include "api/ApiServer.h"
#include "core/BundleCore.h"
#include "routing/QueueBundleEvent.h"
#include "net/BundleReceivedEvent.h"
#include "core/NodeEvent.h"
#include <ibrdtn/data/AgeBlock.h>
#include <ibrcommon/Logger.h>
#include <typeinfo>
#include <algorithm>

#ifdef WITH_COMPRESSION
#include <ibrdtn/data/CompressedPayloadBlock.h>
#endif

#ifdef WITH_BUNDLE_SECURITY
#include "security/SecurityManager.h"
#endif

namespace dtn
{
	namespace api
	{
		ApiServer::ApiServer(const ibrcommon::File &socket)
		 : _srv(socket)
		{
		}

		ApiServer::ApiServer(const ibrcommon::vinterface &net, int port)
		 : _srv()
		{
			_srv.bind(net, port);
		}

		ApiServer::~ApiServer()
		{
			join();
		}

		bool ApiServer::__cancellation()
		{
			_srv.shutdown();
			return true;
		}

		void ApiServer::componentUp()
		{
			_srv.listen(5);
			bindEvent(dtn::routing::QueueBundleEvent::className);
			bindEvent(dtn::core::NodeEvent::className);
		}

		void ApiServer::componentRun()
		{
			try {
				while (true)
				{
					// accept the next client
					ibrcommon::tcpstream *conn = _srv.accept();

					// generate some output
					IBRCOMMON_LOGGER_DEBUG(5) << "new connected client at the extended API server" << IBRCOMMON_LOGGER_ENDL;

					// send welcome banner
					(*conn) << "IBR-DTN " << dtn::daemon::Configuration::getInstance().version() << " API 1.0" << std::endl;

					ibrcommon::MutexLock l(_connection_lock);

					// create a new registration
					Registration reg; _registrations.push_back(reg);
					IBRCOMMON_LOGGER_DEBUG(5) << "new registration " << reg.getHandle() << IBRCOMMON_LOGGER_ENDL;

					ClientHandler *obj = new ClientHandler(*this, _registrations.back(), conn);
					_connections.push_back(obj);

					// start the client handler
					obj->start();

					// breakpoint
					ibrcommon::Thread::yield();
				}
			} catch (const std::exception&) {
				// ignore all errors
				return;
			}
		}

		void ApiServer::componentDown()
		{
			unbindEvent(dtn::routing::QueueBundleEvent::className);
			unbindEvent(dtn::core::NodeEvent::className);

			{
				ibrcommon::MutexLock l(_connection_lock);

				// shutdown all clients
				for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					(*iter)->stop();
				}
			}

			// wait until all clients are down
			while (_connections.size() > 0) ::sleep(1);
		}

		void ApiServer::processIncomingBundle(const dtn::data::EID &source, dtn::data::Bundle &bundle)
		{
			// check address fields for "api:me", this has to be replaced
			static const dtn::data::EID clienteid("api:me");

			// set the source address to the sending EID
			bundle._source = source;

			if (bundle._destination == clienteid) bundle._destination = source;
			if (bundle._reportto == clienteid) bundle._reportto = source;
			if (bundle._custodian == clienteid) bundle._custodian = source;

			// if the timestamp is not set, add a ageblock
			if (bundle._timestamp == 0)
			{
				// check for ageblock
				try {
					bundle.getBlock<dtn::data::AgeBlock>();
				} catch (const dtn::data::Bundle::NoSuchBlockFoundException&) {
					// add a new ageblock
					bundle.push_front<dtn::data::AgeBlock>();
				}
			}

#ifdef WITH_COMPRESSION
			// if the compression bit is set, then compress the bundle
			if (bundle.get(dtn::data::PrimaryBlock::IBRDTN_REQUEST_COMPRESSION))
			{
				try {
					dtn::data::CompressedPayloadBlock::compress(bundle, dtn::data::CompressedPayloadBlock::COMPRESSION_ZLIB);
				} catch (const ibrcommon::Exception &ex) {
					IBRCOMMON_LOGGER(warning) << "compression of bundle failed: " << ex.what() << IBRCOMMON_LOGGER_ENDL;
				};
			}
#endif

#ifdef WITH_BUNDLE_SECURITY
			// if the encrypt bit is set, then try to encrypt the bundle
			if (bundle.get(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT))
			{
				try {
					dtn::security::SecurityManager::getInstance().encrypt(bundle);

					bundle.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_ENCRYPT, false);
				} catch (const dtn::security::SecurityManager::KeyMissingException&) {
					// sign requested, but no key is available
					IBRCOMMON_LOGGER(warning) << "No key available for encrypt process." << IBRCOMMON_LOGGER_ENDL;
				} catch (const dtn::security::SecurityManager::EncryptException&) {
					IBRCOMMON_LOGGER(warning) << "Encryption of bundle failed." << IBRCOMMON_LOGGER_ENDL;
				}
			}

			// if the sign bit is set, then try to sign the bundle
			if (bundle.get(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN))
			{
				try {
					dtn::security::SecurityManager::getInstance().sign(bundle);

					bundle.set(dtn::data::PrimaryBlock::DTNSEC_REQUEST_SIGN, false);
				} catch (const dtn::security::SecurityManager::KeyMissingException&) {
					// sign requested, but no key is available
					IBRCOMMON_LOGGER(warning) << "No key available for sign process." << IBRCOMMON_LOGGER_ENDL;
				}
			}
#endif

			// raise default bundle received event
			dtn::net::BundleReceivedEvent::raise(source, bundle, true);
		}

		const std::string ApiServer::getName() const
		{
			return "ApiServer";
		}

		void ApiServer::connectionUp(ClientHandler *obj)
		{
			// generate some output
			IBRCOMMON_LOGGER_DEBUG(5) << "api connection up" << IBRCOMMON_LOGGER_ENDL;
		}

		void ApiServer::connectionDown(ClientHandler *obj)
		{
			// generate some output
			IBRCOMMON_LOGGER_DEBUG(5) << "api connection down" << IBRCOMMON_LOGGER_ENDL;

			ibrcommon::MutexLock l(_connection_lock);

			// remove this object out of the list
			for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
			{
				if (obj == (*iter))
				{
					_connections.erase(iter);
					break;
				}
			}
		}

		void ApiServer::freeRegistration(Registration &reg)
		{
			// remove the registration
			for (std::list<dtn::api::Registration>::iterator iter = _registrations.begin(); iter != _registrations.end(); iter++)
			{
				if (reg == (*iter))
				{
					IBRCOMMON_LOGGER_DEBUG(5) << "release registration " << reg.getHandle() << IBRCOMMON_LOGGER_ENDL;
					_registrations.erase(iter);
					break;
				}
			}
		}

		void ApiServer::raiseEvent(const dtn::core::Event *evt)
		{
			try {
				const dtn::routing::QueueBundleEvent &queued = dynamic_cast<const dtn::routing::QueueBundleEvent&>(*evt);

				ibrcommon::MutexLock l(_connection_lock);
				for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					ClientHandler &conn = **iter;
					if (conn.getRegistration().hasSubscribed(queued.bundle.destination))
					{
						conn.getRegistration().notify(Registration::NOTIFY_BUNDLE_AVAILABLE);
					}
				}
			} catch (const std::bad_cast&) { };

			try {
				const dtn::core::NodeEvent &ne = dynamic_cast<const dtn::core::NodeEvent&>(*evt);

				ibrcommon::MutexLock l(_connection_lock);
				for (std::list<ClientHandler*>::iterator iter = _connections.begin(); iter != _connections.end(); iter++)
				{
					ClientHandler &conn = **iter;

					if (ne.getAction() == NODE_AVAILABLE)
					{
						conn.eventNodeAvailable(ne.getNode());
					}
					else if (ne.getAction() == NODE_UNAVAILABLE)
					{
						conn.eventNodeUnavailable(ne.getNode());
					}
				}
			} catch (const std::bad_cast&) { };
		}
	}
}
