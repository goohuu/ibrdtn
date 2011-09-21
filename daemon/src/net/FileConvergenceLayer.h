/*
 * FileConvergenceLayer.h
 *
 *  Created on: 20.09.2011
 *      Author: morgenro
 */

#include "Component.h"
#include "net/ConvergenceLayer.h"
#include "core/Node.h"
#include "core/EventReceiver.h"

#ifndef FILECONVERGENCELAYER_H_
#define FILECONVERGENCELAYER_H_

namespace dtn
{
	namespace net
	{

		class FileConvergenceLayer : public dtn::net::ConvergenceLayer, public dtn::daemon::IntegratedComponent, public dtn::core::EventReceiver
		{
		public:
			FileConvergenceLayer();
			virtual ~FileConvergenceLayer();

			void raiseEvent(const dtn::core::Event *evt);

			dtn::core::Node::Protocol getDiscoveryProtocol() const;

			void open(const dtn::core::Node&);
			void queue(const dtn::core::Node &n, const ConvergenceLayer::Job &job);

			const std::string getName() const;

		protected:
			void componentUp();
			void componentDown();

		private:
			static ibrcommon::File getPath(const dtn::core::Node&);
			static std::list<dtn::data::MetaBundle> scan(const ibrcommon::File &path);
			static void load(const dtn::core::Node&);

		};

	} /* namespace net */
} /* namespace dtn */
#endif /* FILECONVERGENCELAYER_H_ */
