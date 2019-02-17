/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __LUASYSTEM_FILE_H__
#define __LUASYSTEM_FILE_H__

#include "luasystem.h"
#include <fsys/filesystem.h>
#include <fsys/fsys_searchflags.hpp>

namespace Lua
{
	DLLLUA StatusCode LoadFile(lua_State *lua,std::string &fInOut,fsys::SearchFlags includeFlags=fsys::SearchFlags::All,fsys::SearchFlags excludeFlags=fsys::SearchFlags::None);
};

#endif
