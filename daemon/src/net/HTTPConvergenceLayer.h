/*
 * HTTPConvergenceLayer.h
 *
 *  Created on: 29.07.2010
 *      Author: morgenro
 */

#ifndef HTTPCONVERGENCELAYER_H_
#define HTTPCONVERGENCELAYER_H_

#include "Component.h"
#include "net/ConvergenceLayer.h"
#include <ibrcommon/Exceptions.h>
#include <ibrcommon/net/NetInterface.h>
#include <ibrcommon/thread/Mutex.h>
#include <iostream>

namespace dtn
{
	namespace net
	{
		class HTTPConvergenceLayer : public ConvergenceLayer, public dtn::daemon::IndependentComponent
		{
		public:
			HTTPConvergenceLayer(const std::string &server);
			virtual ~HTTPConvergenceLayer();

			dtn::core::Node::Protocol getDiscoveryProtocol() const;
			void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

			/**
			 * @see Component::getName()
			 */
			virtual const std::string getName() const;

		protected:
			virtual void componentUp();
			virtual void componentRun();
			virtual void componentDown();
			bool __cancellation();

		private:
			ibrcommon::Mutex _write_lock;
			const std::string _server;
		};
	}
}

#endif /* HTTPCONVERGENCELAYER_H_ */
