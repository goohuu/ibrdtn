/*
 * LuaApplication.cpp
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "daemon/LuaApplication.h"

#ifdef HAVE_LIBLUA5_1

#include <unistd.h>

#ifdef DO_DEBUG_OUTPUT
#include <iostream>
using namespace std;
#endif

namespace dtn
{
	namespace lua
	{
		LuaApplication::LuaApplication(lua_State *L) {
		}

		int LuaApplication::setUser(lua_State *L) {
			unsigned int user = 0;

#ifdef DO_DEBUG_OUTPUT
			cout << "Switching UID to " << user << endl;
#endif

			setuid( user );
			return 0;
		}

		int LuaApplication::setGroup(lua_State *L) {
			unsigned int group = 0;

#ifdef DO_DEBUG_OUTPUT
			cout << "Switching GID to " << group << endl;
#endif

			setgid( group );
			return 0;
		}

		LuaApplication::~LuaApplication() {
		}

		const char LuaApplication::className[] = "Application";
		const Luna<LuaApplication>::RegType LuaApplication::Register[] = {
				{ "setUser", &LuaApplication::setUser },
				{ "setGroup", &LuaApplication::setGroup },
				{ 0 }
		};
	}
}

#endif
