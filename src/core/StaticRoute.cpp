#include "core/StaticRoute.h"
#include <iostream>

namespace dtn
{
	namespace core
	{
		StaticRoute::StaticRoute(string route, string dest)
			: m_route(route), m_dest(dest)
		{
			// prepare for fast matching
			size_t splitter = m_route.find_first_of("*");

			// four types of matching possible
			// 0. no asterisk exists, do exact matching
			// 1. asterisk at the begin
			// 2. asterisk in the middle
			// 3. asterisk at the end
			if ( splitter == string::npos )					m_matchmode = 0;
			else if ( splitter == 0 ) 						m_matchmode = 1;
			else if ( splitter == (m_route.length() -1) ) 	m_matchmode = 3;
			else 											m_matchmode = 2;

			switch (m_matchmode)
			{
				default:

				break;

				case 1:
					m_route1 = m_route.substr(1);
				break;

				case 2:
					m_route1 = m_route.substr(0, splitter);
					m_route2 = m_route.substr(splitter + 1);
				break;

				case 3:
					m_route1 = m_route.substr(0, m_route.length() - 1 );
				break;
			}
		}

		StaticRoute::~StaticRoute()
		{
		}

		bool StaticRoute::match(const dtn::data::Bundle &b)
		{
			string dest = b._destination.getString();

			switch (m_matchmode)
			{
				default:
					// exact matching
					if ( dest == m_route ) return true;
				break;

				case 3:
					// asterisk at the end. the begining must be equal.
					if ( dest.find(m_route1, 0) == 0 ) return true;
				break;

				case 2:
					if (
						 ( dest.find(m_route1) == 0 ) &&
						 ( dest.rfind(m_route2) == (dest.length() - m_route2.length()) )
					   )
						return true;
				break;

				case 1:
					// asterisk at begin. the end must be equal.
					if ( dest.rfind(m_route1) == (dest.length() - m_route1.length()) ) return true;
				break;
			}

			return false;
		}

		string StaticRoute::getDestination()
		{
			return m_dest;
		}
	}
}
