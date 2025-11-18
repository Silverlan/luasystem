// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "lua_headers.hpp"

export module pragma.lua:interface;

export import luabind;
export import std.compat;

#undef RegisterLibrary

export namespace Lua {
	struct DLLLUA IncludeCache {
		bool Contains(const std::string_view &path) const;
		void Add(const std::string_view &path);
		void Clear();
	  private:
		std::unordered_set<uint32_t> m_cache;
	};

	class DLLLUA Interface {
	  public:
		Interface();
		virtual ~Interface();
		const lua_State *GetState() const;
		lua_State *GetState();
		void Open();

		void SetIdentifier(const std::string &identifier);
		const std::string &GetIdentifier() const;
		IncludeCache &GetIncludeCache();

		// These need a const char* which exists for the lifetime of the lua state! (std::string won't work!)
		luabind::module_ &RegisterLibrary(const char *name, const std::shared_ptr<luabind::module_> &mod);
		luabind::module_ &RegisterLibrary(const char *name, const std::unordered_map<std::string, int (*)(lua_State *)> &functions = {});
	  protected:
		lua_State *m_state = nullptr;
		std::string m_identifier;
		std::unordered_map<std::string, std::shared_ptr<luabind::module_>> m_modules;
		IncludeCache m_luaIncludeCache;
	};
};
