/*
 * StaticRoutingExension.h
 *
 *  Created on: 15.02.2010
 *      Author: morgenro
 */

#ifndef STATICROUTINGEXTENSION_H_
#define STATICROUTINGEXTENSION_H_

#include "routing/BaseRouter.h"
#include "routing/MetaBundle.h"

namespace dtn
{
	namespace routing
	{
		class StaticRoutingExtension : public BaseRouter::Extension
		{
		public:
			class StaticRoute
			{
				public:
				StaticRoute(string route, string dest);
				virtual ~StaticRoute();

				bool match(const dtn::data::EID &eid);
				string getDestination();

				private:
				string m_route;
				string m_dest;
				string m_route1;
				string m_route2;

				int m_matchmode;
			};

			StaticRoutingExtension(list<StaticRoutingExtension::StaticRoute> routes);
			~StaticRoutingExtension();

			void notify(const dtn::core::Event *evt);

		private:
			void route(const dtn::routing::MetaBundle &meta);

			list<StaticRoutingExtension::StaticRoute> _routes;
		};
	}
}

#endif /* STATICROUTINGEXTENSION_H_ */
