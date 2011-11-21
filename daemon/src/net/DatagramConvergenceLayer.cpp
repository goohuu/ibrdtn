/*
 * DatagramConvergenceLayer.cpp
 *
 *  Created on: 21.11.2011
 *      Author: morgenro
 */

#include "net/DatagramConvergenceLayer.h"
#include "net/DatagramConnection.h"

#include "core/BundleCore.h"
#include "core/TimeEvent.h"

#include <ibrcommon/net/lowpanstream.h>
#include <ibrcommon/net/UnicastSocketLowpan.h>
#include <ibrcommon/net/lowpansocket.h>

#include <ibrcommon/Logger.h>
#include <ibrcommon/thread/MutexLock.h>
#include <ibrcommon/TimeMeasurement.h>

#include <ibrdtn/data/ScopeControlHopLimitBlock.h>

#include <sys/socket.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <list>

namespace dtn
{
	namespace net
	{
		DatagramConvergenceLayer::DatagramConvergenceLayer(DatagramService &ds)
		 : DiscoveryAgent(dtn::daemon::Configuration::getInstance().getDiscovery()), _service(ds)
		{
		}

		DatagramConvergenceLayer::~DatagramConvergenceLayer()
		{
			join();
		}

		dtn::core::Node::Protocol DatagramConvergenceLayer::getDiscoveryProtocol() const
		{
			return _service.getProtocol();
		}

		void DatagramConvergenceLayer::update(const ibrcommon::vinterface &iface, std::string &name, std::string &params) throw(dtn::net::DiscoveryServiceProvider::NoServiceHereException)
		{
			if (iface == _service.getInterface())
			{
				name = _service.getServiceTag();
				params = _service.getServiceDescription();
			}
			else
			{
				 throw dtn::net::DiscoveryServiceProvider::NoServiceHereException();
			}
		}

		void DatagramConvergenceLayer::send(const std::string &destination, const char *buf, int len)
		{
			// only on sender at once
			ibrcommon::MutexLock l(_send_lock);

			// forward the send request to DatagramService
			_service.send(buf, len);
		}

		void DatagramConvergenceLayer::callback_send(DatagramConnection &connection, const std::string &destination, const char *buf, int len)
		{
			// forward to send method
			send(destination, buf, len);
		}

		void DatagramConvergenceLayer::queue(const dtn::core::Node &node, const ConvergenceLayer::Job &job)
		{
			const std::list<dtn::core::Node::URI> uri_list = node.get(_service.getProtocol());
			if (uri_list.empty()) return;

			// get the first element of the result
			const dtn::core::Node::URI &uri = uri_list.front();

			// some debugging
			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConvergenceLayer::queue"<< IBRCOMMON_LOGGER_ENDL;

			// lock the connection list while working with it
			ibrcommon::MutexLock lc(_connection_lock);

			// get a new or the existing connection for this address
			DatagramConnection &conn = getConnection( uri.value );

			// queue the job to the connection
			conn.queue(job);
		}

		DatagramConnection& DatagramConvergenceLayer::getConnection(const std::string &identifier)
		{
			// Test if connection for this address already exist
			for(std::list<DatagramConnection*>::iterator i = _connections.begin(); i != _connections.end(); ++i)
			{
				IBRCOMMON_LOGGER_DEBUG(10) << "Connection identifier: " << (*i)->getIdentifier() << IBRCOMMON_LOGGER_ENDL;
				if ((*i)->getIdentifier() == identifier)
					return *(*i);
			}

			// Connection does not exist, create one and put it into the list
			DatagramConnection *connection = new DatagramConnection(identifier, _service.getMaxMessageSize() - 1, (*this));

			_connections.push_back(connection);
			IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConvergenceLayer::getConnection "<< connection->getIdentifier() << IBRCOMMON_LOGGER_ENDL;
			connection->start();
			return *connection;
		}

		void DatagramConvergenceLayer::remove(const DatagramConnection &conn)
		{
			ibrcommon::MutexLock lc(_connection_lock);

			std::list<DatagramConnection*>::iterator i;
			for(i = _connections.begin(); i != _connections.end(); ++i)
			{
				if ((*i) == (&conn))
				{
					_connections.erase(i);
					return;
				}
			}
		}

		void DatagramConvergenceLayer::componentUp()
		{
			bindEvent(dtn::core::TimeEvent::className);
			try {
				_service.bind();
				addService(this);
			} catch (const std::exception &e) {
				IBRCOMMON_LOGGER_DEBUG(10) << "Failed to add DatagramConvergenceLayer on " << _service.getInterface().toString() << IBRCOMMON_LOGGER_ENDL;
				IBRCOMMON_LOGGER_DEBUG(10) << "Exception: " << e.what() << IBRCOMMON_LOGGER_ENDL;
			}
		}

		void DatagramConvergenceLayer::componentDown()
		{
			unbindEvent(dtn::core::TimeEvent::className);
		}

		void DatagramConvergenceLayer::sendAnnoucement(const u_int16_t &sn, std::list<dtn::net::DiscoveryService> &services)
		{
			IBRCOMMON_LOGGER(notice) << "DatagramConvergenceLayer IPND beacon send started" << IBRCOMMON_LOGGER_ENDL;

			DiscoveryAnnouncement announcement(DiscoveryAnnouncement::DISCO_VERSION_01, dtn::core::BundleCore::local);

			// set sequencenumber
			announcement.setSequencenumber(sn);

			// clear all services
			announcement.clearServices();

			// add services
			for (std::list<DiscoveryService>::iterator iter = services.begin(); iter != services.end(); iter++)
			{
				DiscoveryService &service = (*iter);

				try {
					// update service information
					service.update(_service.getInterface());

					// add service to discovery message
					announcement.addService(service);
				} catch (const dtn::net::DiscoveryServiceProvider::NoServiceHereException&) {

				}
			}

			// serialize announcement
			stringstream ss;
			ss << announcement;

			int len = ss.str().size();
			send("", ss.str().c_str(), len);
		}

		void DatagramConvergenceLayer::componentRun()
		{
			_running = true;

			size_t maxlen = _service.getMaxMessageSize();
			std::string address;
			char data[maxlen];
			char header = 0;

			while (_running)
			{
				IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConvergenceLayer::componentRun early" << IBRCOMMON_LOGGER_ENDL;

				// Receive full frame from socket
				size_t len = _service.recvfrom(data, maxlen, address);

				IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConvergenceLayer::componentRun" << IBRCOMMON_LOGGER_ENDL;

				// We got nothing from the socket, keep reading
				if (len == 0) continue;

				// Retrieve header of frame
				header = data[0];

				IBRCOMMON_LOGGER_DEBUG(10) << "DatagramConvergenceLayer::componentRun: ADDRESS " << address << IBRCOMMON_LOGGER_ENDL;

				// Check for extended header and retrieve if available
				if (header & HEADER_BROADCAST)
				{
					IBRCOMMON_LOGGER(notice) << "Received announcement for DatagramConvergenceLayer discovery" << IBRCOMMON_LOGGER_ENDL;
					DiscoveryAnnouncement announce;
					stringstream ss;
					ss.write(data+1, len-1);
					ss >> announce;
					DiscoveryAgent::received(announce, 30);
					continue;
				}

				ibrcommon::MutexLock lc(_connection_lock);

				// Connection instance for this address
				DatagramConnection& connection = getConnection(address);

				// Decide in which queue to write based on the src address
				connection.queue(data+1, len-1);

				yield();
			}
		}

		void DatagramConvergenceLayer::raiseEvent(const Event *evt)
		{
			try {
				const TimeEvent &time=dynamic_cast<const TimeEvent&>(*evt);
				if (time.getAction() == TIME_SECOND_TICK)
					if (time.getTimestamp() % 5 == 0)
						timeout();
			} catch (const std::bad_cast&)
			{}
		}

		bool DatagramConvergenceLayer::__cancellation()
		{
			_running = false;
			_service.shutdown();

			return true;
		}

		const std::string DatagramConvergenceLayer::getName() const
		{
			return "DatagramConvergenceLayer";
		}
	} /* namespace data */
} /* namespace dtn */
