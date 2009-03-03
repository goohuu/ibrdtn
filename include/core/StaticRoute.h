#ifndef STATICROUTE_H_
#define STATICROUTE_H_

#include "data/Bundle.h"

namespace dtn
{
	namespace core
	{
		class StaticRoute
		{
			public:
			StaticRoute(string route, string dest);
			~StaticRoute();

			bool match(dtn::data::Bundle *b);
			string getDestination();

			private:
			string m_route;
			string m_dest;
			string m_route1;
			string m_route2;

			int m_matchmode;
		};
	}
}

#endif /*STATICROUTE_H_*/
