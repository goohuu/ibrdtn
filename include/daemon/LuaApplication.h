/*
 * LuaApplication.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef LUAAPPLICATION_H_
#define LUAAPPLICATION_H_

#ifdef HAVE_LIBLUA5_1

#include "utils/Luna.h"

namespace dtn
{
	namespace lua
	{
		class LuaApplication
		{
			public:
				LuaApplication(lua_State *L);
				~LuaApplication();

				int setUser(lua_State *L);
				int setGroup(lua_State *L);

				static const char className[];
				static const Luna<LuaApplication>::RegType Register[];
		};
	}
}

#endif

#endif /* LUAAPPLICATION_H_ */
