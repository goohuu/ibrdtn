/*
 * LuaConfiguration.h
 *
 *  Created on: 04.03.2009
 *      Author: morgenro
 */

#include "config.h"

#ifndef LUACONFIGURATION_H_
#define LUACONFIGURATION_H_

#ifdef HAVE_LIBLUA5_1

#include "utils/Luna.h"

namespace dtn
{
	namespace lua
	{
		class LuaConfiguration
		{
			public:
				LuaConfiguration(lua_State *L);
				~LuaConfiguration();

				int foo(lua_State *L);

				static const char className[];
				static const Luna<LuaConfiguration>::RegType Register[];
		};
	}
}

#endif

#endif /* LUACONFIGURATION_H_ */
