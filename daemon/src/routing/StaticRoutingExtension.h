/*
 * StaticRoutingExension.h
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#ifndef STATICROUTINGEXTENSION_H_
#define STATICROUTINGEXTENSION_H_

#include "routing/BaseRouter.h"
#include <ibrdtn/data/MetaBundle.h>
#include <ibrcommon/thread/Queue.h>

namespace dtn
{
	namespace routing
	{
		class StaticRoutingExtension : public BaseRouter::ThreadedExtension
		{
		public:
			class StaticRoute
			{
			public:
				StaticRoute(const std::string &route, const std::string &dest);
				virtual ~StaticRoute();

				bool match(const dtn::data::EID &eid) const;
				const dtn::data::EID& getDestination() const;

			private:
				std::string m_route;
				const dtn::data::EID m_dest;
				std::string m_route1;
				std::string m_route2;

				int m_matchmode;
			};

			StaticRoutingExtension(const std::list<StaticRoutingExtension::StaticRoute> &routes);
			virtual ~StaticRoutingExtension();

			void notify(const dtn::core::Event *evt);

			virtual void stopExtension();

		protected:
			void run();
			bool __cancellation();

		private:
			class Task
			{
			public:
				virtual ~Task() {};
				virtual std::string toString() = 0;
			};

			class SearchNextBundleTask : public Task
			{
			public:
				SearchNextBundleTask(const dtn::data::EID &eid);
				virtual ~SearchNextBundleTask();

				virtual std::string toString();

				const dtn::data::EID eid;
			};

			class ProcessBundleTask : public Task
			{
			public:
				ProcessBundleTask(const dtn::data::MetaBundle &meta, const dtn::data::EID &origin);
				virtual ~ProcessBundleTask();

				virtual std::string toString();

				const dtn::data::MetaBundle bundle;
				const dtn::data::EID origin;
			};

			/**
			 * hold queued tasks for later processing
			 */
			ibrcommon::Queue<StaticRoutingExtension::Task* > _taskqueue;

			/**
			 * static list of routes
			 */
			const std::list<StaticRoutingExtension::StaticRoute> _routes;
		};
	}
}

#endif /* STATICROUTINGEXTENSION_H_ */
