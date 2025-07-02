// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __LUASYSTEM_FILE_H__
#define __LUASYSTEM_FILE_H__

#include "luasystem.h"
#include <fsys/filesystem.h>
#include <fsys/fsys_searchflags.hpp>

namespace Lua {
	DLLLUA StatusCode LoadFile(lua_State *lua, std::string &fInOut, fsys::SearchFlags includeFlags = fsys::SearchFlags::All, fsys::SearchFlags excludeFlags = fsys::SearchFlags::None);
};

#endif
