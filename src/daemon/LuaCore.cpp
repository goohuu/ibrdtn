/*
 * LuaCommands.cpp
 *
 *  Created on: 04.03.2009
 *      Author: morgenro
 */

#include "daemon/LuaCore.h"

#ifdef HAVE_LIBLUA5_1

#include "daemon/LuaBundleCore.h"
#include "daemon/LuaApplication.h"

namespace dtn
{
	namespace lua
	{
		LuaCore::LuaCore()
		{
			m_L = lua_open();

			// load the libs
			luaL_openlibs(m_L);

			// register the commands
			registerAllCommands();
		}

		LuaCore::~LuaCore()
		{
			lua_close(m_L);
		}

		void LuaCore::run(string filename)
		{
			//run a Lua script here
			luaL_dofile(m_L, filename.c_str());
		}

		void LuaCore::registerAllCommands()
		{
			Luna<LuaApplication>::Register(m_L);
			Luna<LuaBundleCore>::Register(m_L);
		}

		int LuaCore::nodeAvailable(lua_State *L)
		{
			/* push the sum */
			lua_pushnumber(L, 1);

			return 1;
		}

		int LuaCore::nodeUnavailable(lua_State *L)
		{
			/* push the sum */
			lua_pushnumber(L, 2);

			return 1;
		}
	}
}

#endif
