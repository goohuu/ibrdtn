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

namespace dtn
{
	namespace net
	{
		class DatagramService
		{
		public:
			virtual ~DatagramService() {};
			virtual void bind() = 0;
			virtual void shutdown() = 0;
			virtual void send(const char *buf, size_t length) = 0;
			virtual size_t recvfrom(char *buf, size_t length, std::string &address) = 0;

			virtual size_t getMaxMessageSize() const = 0;
			virtual const std::string getServiceTag() const = 0;
			virtual const std::string getServiceDescription() const = 0;
			virtual const ibrcommon::vinterface& getInterface() const = 0;
			virtual dtn::core::Node::Protocol getProtocol() const = 0;
		};

		class DatagramConvergenceLayer : public DiscoveryAgent, public ConvergenceLayer, public dtn::daemon::IndependentComponent, public EventReceiver, public DiscoveryServiceProvider, public DatagramConnectionCallback
		{
		public:
			enum HEADER_FLAGS
			{
				HEADER_BROADCAST = 1
			};

			DatagramConvergenceLayer(DatagramService &ds);
			virtual ~DatagramConvergenceLayer();

			/**
			 * this method updates the given values
			 */
			void update(const ibrcommon::vinterface &iface, std::string &name, std::string &data) throw(dtn::net::DiscoveryServiceProvider::NoServiceHereException);

			/**
			 * Returns the protocol identifier
			 * @return
			 */
			dtn::core::Node::Protocol getDiscoveryProtocol() const;

			/**
			 * Queueing a job for a specific node. Starting point for the DTN core to submit bundles to nodes behind the LoWPAN CL
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

			/**
			 * callback send for connections
			 * @param connection
			 * @param destination
			 * @param buf
			 * @param len
			 */
			void callback_send(DatagramConnection &connection, const std::string &destination, const char *buf, int len);

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();
			bool __cancellation();

			virtual void sendAnnoucement(const u_int16_t &sn, std::list<dtn::net::DiscoveryService> &services);

		private:
			DatagramConnection& getConnection(const std::string &identifier);
			void remove(const DatagramConnection &conn);
			void send(const std::string &destination, const char *buf, int len);

			DatagramService &_service;

			ibrcommon::Mutex _send_lock;

			ibrcommon::Mutex _connection_lock;
			std::list<DatagramConnection*> _connections;

			bool _running;
		};
	} /* namespace data */
} /* namespace dtn */
#endif /* DATAGRAMCONVERGENCELAYER_H_ */
