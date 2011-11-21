/*
 * DatagramConvergenceLayer.h
 *
 *  Created on: 21.11.2011
 *      Author: morgenro
 */

#ifndef DATAGRAMCONVERGENCELAYER_H_
#define DATAGRAMCONVERGENCELAYER_H_

#include "Component.h"
#include "net/DatagramConnection.h"
#include "net/ConvergenceLayer.h"
#include "net/DiscoveryAgent.h"
#include "net/DiscoveryServiceProvider.h"
#include "core/Node.h"
#include <ibrcommon/net/vinterface.h>

#include <list>

namespace dtn
{
	namespace net
	{
		class DatagramService
		{
		public:
			virtual ~DatagramService() {};

			/**
			 * Bind to the local socket.
			 */
			virtual void bind() = 0;

			/**
			 * Shutdown the socket. Unblock all calls on the socket (recv, send, etc.)
			 */
			virtual void shutdown() = 0;

			/**
			 * Send the payload as datagram to a defined destination
			 * @param address The destination address encoded as string.
			 * @param buf The buffer to send.
			 * @param length The number of available bytes in the buffer.
			 */
			virtual void send(const std::string &address, const char *buf, size_t length) = 0;

			/**
			 * Send the payload as datagram to all neighbors (broadcast)
			 * @param buf The buffer to send.
			 * @param length The number of available bytes in the buffer.
			 */
			virtual void send(const char *buf, size_t length) = 0;

			/**
			 * Receive an incoming datagram.
			 * @param buf A buffer to catch the incoming data.
			 * @param length The length of the buffer.
			 * @param address A buffer for the address of the sender.
			 * @return The number of received bytes.
			 */
			virtual size_t recvfrom(char *buf, size_t length, std::string &address) = 0;

			/**
			 * Get the maximum message size (MTU) for datagrams of this service.
			 * @return The maximum message size as bytes.
			 */
			virtual size_t getMaxMessageSize() const = 0;

			/**
			 * Get the tag for this service used in discovery messages.
			 * @return The tag as string.
			 */
			virtual const std::string getServiceTag() const = 0;

			/**
			 * Get the service description for this convergence layer. This
			 * data is used to contact this node.
			 * @return The service description as string.
			 */
			virtual const std::string getServiceDescription() const = 0;

			/**
			 * The used interface as vinterface object.
			 * @return A vinterface object.
			 */
			virtual const ibrcommon::vinterface& getInterface() const = 0;

			/**
			 * The protocol identifier for this type of service.
			 * @return
			 */
			virtual dtn::core::Node::Protocol getProtocol() const = 0;
		};

		class DatagramConvergenceLayer : public DiscoveryAgent, public ConvergenceLayer,
				public dtn::daemon::IndependentComponent, public EventReceiver,
				public DiscoveryServiceProvider, public DatagramConnectionCallback
		{
		public:
			enum HEADER_FLAGS
			{
				HEADER_UNKOWN = 0,
				HEADER_BROADCAST = 1,
				HEADER_SEGMENT = 2
			};

			DatagramConvergenceLayer(DatagramService &ds);
			virtual ~DatagramConvergenceLayer();

			/**
			 * this method updates the given values
			 */
			void update(const ibrcommon::vinterface &iface, std::string &name, std::string &data)
				throw(dtn::net::DiscoveryServiceProvider::NoServiceHereException);

			/**
			 * Returns the protocol identifier
			 * @return
			 */
			dtn::core::Node::Protocol getDiscoveryProtocol() const;

			/**
			 * Queueing a job for a specific node. Starting point for the DTN core to submit
			 * bundles to nodes behind the convergence layer
			 * @param n Node reference
			 * @param job Job reference
			 */
			void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

			/**
			 * Public method for event callbacks
			 * @param evt
			 */
			virtual void raiseEvent(const dtn::core::Event *evt);

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();
			bool __cancellation();

			virtual void sendAnnoucement(const u_int16_t &sn, std::list<dtn::net::DiscoveryService> &services);

			/**
			 * callback send for connections
			 * @param connection
			 * @param destination
			 * @param buf
			 * @param len
			 */
			void callback_send(DatagramConnection &connection, const std::string &destination, const char *buf, int len);

			void connectionUp(const DatagramConnection *conn);
			void connectionDown(const DatagramConnection *conn);

		private:
			DatagramConnection& getConnection(const std::string &identifier);

			/**
			 * Send the payload as datagram to a defined destination
			 * @param address The destination address encoded as string.
			 * @param buf The buffer to send.
			 * @param length The number of available bytes in the buffer.
			 */
			void send(const std::string &destination, const char *buf, int len);

			/**
			 * Send the payload as datagram to all neighbors (broadcast)
			 * @param buf The buffer to send.
			 * @param length The number of available bytes in the buffer.
			 */
			void send(const char *buf, int len);

			DatagramService &_service;

			ibrcommon::Mutex _send_lock;

			ibrcommon::Mutex _connection_lock;
			std::list<DatagramConnection*> _connections;

			bool _running;
		};
	} /* namespace data */
} /* namespace dtn */
#endif /* DATAGRAMCONVERGENCELAYER_H_ */
