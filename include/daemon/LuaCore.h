/*
 * LuaCore.h
 *
 *  Created on: 04.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef LUACORE_H_
#define LUACORE_H_

#ifdef HAVE_LIBLUA5_1

extern "C" {
#include "lua5.1/lua.h"
#include "lua5.1/lualib.h"
#include "lua5.1/lauxlib.h"
}

#include <string>
using namespace std;

namespace dtn
{
	namespace lua
	{
		class LuaCore
		{
		public:
			LuaCore();
			~LuaCore();

			void run(string filename);

			void registerAllCommands();
			int nodeAvailable(lua_State *L);
			int nodeUnavailable(lua_State *L);
		private:
			/* the Lua interpreter */
			lua_State* m_L;
		};
	}
}

#endif

#endif /* LUACORE_H_ */
