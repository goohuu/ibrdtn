/*
 * LuaBundleCore.h
 *
 *  Created on: 05.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef LUABUNDLECORE_H_
#define LUABUNDLECORE_H_

#ifdef HAVE_LIBLUA5_1

#include "utils/Luna.h"

namespace dtn
{
	namespace lua
	{
		class LuaBundleCore
		{
			public:
				LuaBundleCore(lua_State *L);
				~LuaBundleCore();

				int setLocalURI(lua_State *L);
				int setLocalisation(lua_State *L);
				int addConvergenceLayer(lua_State *L);
				int addStaticRoute(lua_State *L);
				int addConnection(lua_State *L);
				int setStorage(lua_State *L);

				int startup(lua_State *L);
				int shutdown(lua_State *L);

				static const char className[];
				static const Luna<LuaBundleCore>::RegType Register[];
		};
	}
}

#endif

#endif /* LUABUNDLECORE_H_ */
