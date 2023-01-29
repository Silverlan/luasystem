/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __IMPL_LUAJIT_DEFINITIONS_HPP__
#define __IMPL_LUAJIT_DEFINITIONS_HPP__

#define luaL_checkuserdata(L, n) (luaL_checktype(L, (n), LUA_TUSERDATA))

#ifdef USE_LUAJIT
static inline void luaX52_luaL_setfuncs(lua_State *L, const luaL_Reg *l, int nup)
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
#define luaL_newlibtable(L, l) (lua_createtable(L, 0, sizeof(l)))
#define luaL_setfuncs luaX52_luaL_setfuncs
#define luaL_newlib(L, l) (luaL_newlibtable(L, l), luaL_setfuncs(L, l, 0))
#define lua_rawlen lua_objlen
#endif

#endif
