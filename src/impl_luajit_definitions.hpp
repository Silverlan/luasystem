// SPDX-FileCopyrightText: © 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __IMPL_LUAJIT_DEFINITIONS_HPP__
#define __IMPL_LUAJIT_DEFINITIONS_HPP__

#define luaL_checkuserdata(L, n) (luaL_checktype(L, (n), LUA_TUSERDATA))

//FIXME: DEPENDENCY_LUA points to LuaJIT library.
#if 1
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
#endif

#define lua_rawlen lua_objlen

#endif
