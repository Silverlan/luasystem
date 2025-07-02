// SPDX-FileCopyrightText: © 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __LUAINTERFACE_HPP__
#define __LUAINTERFACE_HPP__

#include "luadefinitions.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>

struct lua_State;
namespace luabind {
	class module_;
};
namespace Lua {
	class DLLLUA Interface {
	  public:
		Interface();
		virtual ~Interface();
		const lua_State *GetState() const;
		lua_State *GetState();
		void Open();

		void SetIdentifier(const std::string &identifier);
		const std::string &GetIdentifier() const;
		std::vector<std::string> &GetIncludeCache();

		// These need a const char* which exists for the lifetime of the lua state! (std::string won't work!)
		luabind::module_ &RegisterLibrary(const char *name, const std::shared_ptr<luabind::module_> &mod);
        luabind::module_ &RegisterLibrary(const char *name, const std::unordered_map<std::string, int (*)(lua_State *)> &functions = {});
	  protected:
		lua_State *m_state = nullptr;
		std::string m_identifier;
		std::unordered_map<std::string, std::shared_ptr<luabind::module_>> m_modules;
		std::vector<std::string> m_luaIncludeCache;
	};
};

#endif
