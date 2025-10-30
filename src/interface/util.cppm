// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "lua_headers.hpp"

export module pragma.lua:util;

export {
	namespace pragma::lua::core {
		DLLLUA void luaX52_luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
		{
			luaL_checkstack(L, nup, "too many upvalues");
			for(; l->name != NULL; l++) {    /* fill the table with given functions */
				for(int i = 0; i < nup; i++) /* copy upvalues to the top */
					lua_pushvalue(L, -nup);
				lua_pushcclosure(L, l->func, nup); /* closure with those upvalues */
				lua_setfield(L, -(nup + 2), l->name);
			}
			lua_pop(L, nup); /* remove upvalues */
		}

		DLLLUA void setfuncs(lua_State *L, const luaL_Reg *l, int nup) {
			luaX52_luaL_setfuncs(L, l, nup);
		}

		DLLLUA void checkuserdata(lua_State *L, int n) {
			luaL_checktype(L, n, LUA_TUSERDATA);
		}

		DLLLUA size_t rawlen(lua_State *L, int idx) {
			return lua_objlen(L, idx);
		}
	}
}
