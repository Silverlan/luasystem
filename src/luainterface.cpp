// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "luainterface.hpp"
#include "luasystem.h"
#include "impl_luajit_definitions.hpp"
#include <sharedutils/util_string.h>
#include <sharedutils/util_path.hpp>

bool Lua::IncludeCache::Contains(const std::string_view &path) const
{
	// TODO: It would be more efficient to generate a path-hash directly instead of copying the string
	auto hash = ustring::string_switch_ci::hash(util::FilePath(path).GetString());
	return m_cache.contains(hash);
}
void Lua::IncludeCache::Add(const std::string_view &path)
{
	auto hash = ustring::string_switch_ci::hash(util::FilePath(path).GetString());
	m_cache.insert(hash);
}
void Lua::IncludeCache::Clear() { m_cache.clear(); }

Lua::Interface::Interface() {}

Lua::Interface::~Interface()
{
	if(m_state != nullptr)
		lua_close(m_state);
}

void Lua::Interface::Open()
{
	if(m_state != nullptr)
		return;
#ifdef USE_LUAJIT
	m_state = lua_open();
#else
	m_state = luaL_newstate();
#endif
}

Lua::IncludeCache &Lua::Interface::GetIncludeCache() { return m_luaIncludeCache; }

void Lua::Interface::SetIdentifier(const std::string &identifier) { m_identifier = identifier; }
const std::string &Lua::Interface::GetIdentifier() const { return m_identifier; }

const lua_State *Lua::Interface::GetState() const { return const_cast<Interface *>(this)->GetState(); }
lua_State *Lua::Interface::GetState() { return m_state; }

luabind::module_ &Lua::Interface::RegisterLibrary(const char *name, const std::shared_ptr<luabind::module_> &mod)
{
	m_modules.insert(std::make_pair(name, mod));
	return *mod;
}
luabind::module_ &Lua::Interface::RegisterLibrary(const char *name, const std::unordered_map<std::string, int (*)(lua_State *)> &functions)
{
	auto it = m_modules.find(name);
	if(it == m_modules.end())
		it = m_modules.insert(std::make_pair(name, std::make_shared<luabind::module_>(luabind::module(m_state, name)))).first;
	auto &mod = it->second;
	if(functions.empty() == false) {
		Lua::GetGlobal(m_state, name);
		auto bExists = Lua::IsTable(m_state, -1);
		if(bExists) {
			// If the library already exists, just add the new functions to it
			auto t = Lua::GetStackTop(m_state);
			for(auto &pair : functions) {
				Lua::PushString(m_state, pair.first);
				Lua::PushCFunction(m_state, pair.second);
				Lua::SetTableValue(m_state, t);
			}
			Lua::Pop(m_state, 1);
		}
		else {
			Lua::Pop(m_state, 1);
			// Library doesn't exist yet; Create it
			std::vector<luaL_Reg> regFuncs;
			regFuncs.reserve(functions.size() + 1);
			for(auto &pair : functions)
				regFuncs.push_back({pair.first.c_str(), pair.second});
			regFuncs.push_back({nullptr, nullptr});
#pragma warning(disable : 4309)
			lua_newtable(m_state);
			luaL_setfuncs(m_state, regFuncs.data(), 0);
#pragma warning(default : 4309)
			lua_setglobal(m_state, name);
		}
	}
	return *mod;
}
