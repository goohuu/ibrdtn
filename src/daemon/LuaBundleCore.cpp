/*
 * LuaBundleCore.cpp
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "daemon/LuaBundleCore.h"

#include <iostream>
using namespace std;

#ifdef HAVE_LIBLUA5_1

namespace dtn
{
	namespace lua
	{
		LuaBundleCore::LuaBundleCore(lua_State *L) {
		}

		LuaBundleCore::~LuaBundleCore() {
		}

		int LuaBundleCore::setLocalURI(lua_State *L)
		{
			return 0;
		}

		int LuaBundleCore::setLocalisation(lua_State *L)
		{
			return 0;
		}

		int LuaBundleCore::addConvergenceLayer(lua_State *L)
		{
			return 0;
		}

		int LuaBundleCore::addStaticRoute(lua_State *L)
		{
			return 0;
		}

		int LuaBundleCore::addConnection(lua_State *L)
		{
			return 0;
		}

		int LuaBundleCore::setStorage(lua_State *L)
		{
			return 0;
		}

		int LuaBundleCore::startup(lua_State *L)
		{
			cout << "startup" << endl;
			return 0;
		}

		int LuaBundleCore::shutdown(lua_State *L)
		{
			cout << "shutdown" << endl;
			return 0;
		}

		const char LuaBundleCore::className[] = "BundleCore";
		const Luna<LuaBundleCore>::RegType LuaBundleCore::Register[] = {
				{ "setLocalURI", &LuaBundleCore::setLocalURI },
				{ "setLocalisation", &LuaBundleCore::setLocalisation },
				{ "addConvergenceLayer", &LuaBundleCore::addConvergenceLayer },
				{ "addStaticRoute", &LuaBundleCore::addStaticRoute },
				{ "addConnection", &LuaBundleCore::addConnection },
				{ "setStorage", &LuaBundleCore::setStorage },
				{ "startup", &LuaBundleCore::startup },
				{ "shutdown", &LuaBundleCore::shutdown },
				{ 0 }
		};
	}
}

#endif
