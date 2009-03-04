/*
 * LuaConfiguration.cpp
 *
 *  Created on: 04.03.2009
 *      Author: morgenro
 */

#include "daemon/LuaConfiguration.h"

#ifdef HAVE_LIBLUA5_1

namespace dtn
{
	namespace lua
	{
		LuaConfiguration::LuaConfiguration(lua_State *L) {
			printf("in constructor\n");
		}

		int LuaConfiguration::foo(lua_State *L) {
			printf("in foo\n");
		}

		LuaConfiguration::~LuaConfiguration() {
			printf("in destructor\n");
		}

		const char LuaConfiguration::className[] = "Configuration";
		const Luna<LuaConfiguration>::RegType LuaConfiguration::Register[] = {
				{ "foo", &LuaConfiguration::foo },
				{ 0 }
		};
	}
}

#endif
