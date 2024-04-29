/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __LUASYSTEM_H__
#define __LUASYSTEM_H__

#include "luadefinitions.h"
extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}
#include <luabind/luabind.hpp>
#include <luabind/operator.hpp>
#include <luabind/detail/conversion_policies/pointer_converter.hpp>
#include <string>
#include <unordered_map>
#include <memory>

#define LUA_REGISTER_TYPE(typeName, internalType)                                                                                                                                                                                                                                                \
	namespace Lua {                                                                                                                                                                                                                                                                              \
		inline auto Check##typeName(lua_State *l, int n)                                                                                                                                                                                                                                         \
		{                                                                                                                                                                                                                                                                                        \
			Lua::CheckUserData(l, n);                                                                                                                                                                                                                                                            \
			luabind::object o(luabind::from_stack(l, n));                                                                                                                                                                                                                                        \
			auto pV = luabind::object_cast_nothrow<internalType *>(o);                                                                                                                                                                                                                           \
			if(pV == boost::none) {                                                                                                                                                                                                                                                              \
				std::string err = #typeName " expected, got ";                                                                                                                                                                                                                                   \
				err += Lua::GetTypeString(l, n);                                                                                                                                                                                                                                                 \
				luaL_argerror(l, n, err.c_str());                                                                                                                                                                                                                                                \
			}                                                                                                                                                                                                                                                                                    \
			return pV.get();                                                                                                                                                                                                                                                                     \
		}                                                                                                                                                                                                                                                                                        \
		inline auto Is##typeName(lua_State *l, int n)                                                                                                                                                                                                                                            \
		{                                                                                                                                                                                                                                                                                        \
			if(!lua_isuserdata(l, n))                                                                                                                                                                                                                                                            \
				return false;                                                                                                                                                                                                                                                                    \
			luabind::object o(luabind::from_stack(l, n));                                                                                                                                                                                                                                        \
			auto pV = luabind::object_cast_nothrow<internalType *>(o);                                                                                                                                                                                                                           \
			return (pV != boost::none) ? true : false;                                                                                                                                                                                                                                           \
		}                                                                                                                                                                                                                                                                                        \
		inline auto Get##typeName(lua_State *l, int n)                                                                                                                                                                                                                                           \
		{                                                                                                                                                                                                                                                                                        \
			luabind::object o(luabind::from_stack(l, n));                                                                                                                                                                                                                                        \
			auto pV = luabind::object_cast_nothrow<internalType *>(o);                                                                                                                                                                                                                           \
			return pV.get();                                                                                                                                                                                                                                                                     \
		}                                                                                                                                                                                                                                                                                        \
	};

#ifndef LUA_OK
#define LUA_OK (decltype(LUA_YIELD)(0))
#endif

struct lua_Debug;
namespace Lua {
	enum class DLLLUA StatusCode : decltype(LUA_OK) {
		Ok = LUA_OK,
		Yield = LUA_YIELD,
		ErrorRun = LUA_ERRRUN,
		ErrorSyntax = LUA_ERRSYNTAX,
		ErrorMemory = LUA_ERRMEM,
#if 0
#ifndef USE_LUAJIT
		ErrorGC = LUA_ERRGCMM,
#endif
#endif
		ErrorErrorHandler = LUA_ERRERR,
		ErrorFile = LUA_ERRFILE
	};
	enum class DLLLUA Type : decltype(LUA_TNONE) { None = LUA_TNONE, Nil = LUA_TNIL, Bool = LUA_TBOOLEAN, LightUserData = LUA_TLIGHTUSERDATA, Number = LUA_TNUMBER, String = LUA_TSTRING, Table = LUA_TTABLE, Function = LUA_TFUNCTION, UserData = LUA_TUSERDATA, Thread = LUA_TTHREAD };

	constexpr std::string SCRIPT_DIRECTORY = "lua";
	constexpr std::string SCRIPT_DIRECTORY_SLASH = "lua/";
	constexpr std::string FILE_EXTENSION = "lua";
	constexpr std::string FILE_EXTENSION_PRECOMPILED = "luac";
	constexpr std::string DOT_FILE_EXTENSION = ".lua";
	constexpr std::string DOT_FILE_EXTENSION_PRECOMPILED = ".luac";

	template<typename T>
	using base_type = typename std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
	template<typename T>
	concept is_native_type = std::is_arithmetic_v<T> || std::is_same_v<base_type<T>, std::string> || std::is_same_v<T, const char *>;

	const auto RegistryIndex = LUA_REGISTRYINDEX;
	DLLLUA lua_State *CreateState();
	DLLLUA void CloseState(lua_State *lua);
	DLLLUA void Call(lua_State *lua, int32_t nargs, int32_t nresults);
	// Function and function arguments have to be pushed inside 'pushFuncArgs'-callback!
	DLLLUA StatusCode ProtectedCall(lua_State *lua, const std::function<StatusCode(lua_State *)> &pushFuncArgs, int32_t numResults, int32_t (*traceback)(lua_State *) = nullptr);
	DLLLUA StatusCode ProtectedCall(lua_State *lua, int32_t nargs, int32_t nresults, std::string *err = nullptr);
	// Creates a new table and pushes it onto the stack.
	DLLLUA int32_t CreateTable(lua_State *lua);
	DLLLUA int32_t CreateMetaTable(lua_State *lua, const std::string &tname);

	DLLLUA bool IsBool(lua_State *lua, int32_t idx);
	DLLLUA bool IsCFunction(lua_State *lua, int32_t idx);
	DLLLUA bool IsFunction(lua_State *lua, int32_t idx);
	DLLLUA bool IsNil(lua_State *lua, int32_t idx);
	DLLLUA bool IsNone(lua_State *lua, int32_t idx);
	DLLLUA bool IsSet(lua_State *lua, int32_t idx);
	DLLLUA bool IsNumber(lua_State *lua, int32_t idx);
	DLLLUA bool IsString(lua_State *lua, int32_t idx);
	DLLLUA bool IsTable(lua_State *lua, int32_t idx);
	DLLLUA bool IsUserData(lua_State *lua, int32_t idx);

	DLLLUA std::string get_source(const lua_Debug &dbgInfo);
	DLLLUA std::string get_short_source(const lua_Debug &dbgInfo);

	//this won't work. Compiler will emit diffrent symbols.
	//Might be pre C++20 remnant.
	//template<class T>
	//void Push(lua_State *lua,const T &value);
	template<class T>
	    requires(is_native_type<T>)
	void Push(lua_State *lua, T value)
	{
		if constexpr(std::is_pointer_v<T>) {
			if(!value)
				lua_pushnil(lua);
			else
				Push(lua, *value);
		}
		else {
			using TBase = base_type<T>;
			if constexpr(std::is_same_v<TBase, bool>)
				lua_pushboolean(lua, value);
			else if constexpr(std::is_integral_v<TBase>)
				lua_pushinteger(lua, value);
			else if constexpr(std::is_arithmetic_v<TBase>)
				lua_pushnumber(lua, value);
			else if constexpr(std::is_same_v<TBase, std::string>)
				lua_pushstring(lua, value.c_str());
			else
				lua_pushstring(lua, value);
		}
	}
	template<class T>
	    requires(!is_native_type<T>)
	void Push(lua_State *lua, const T &value)
	{
		luabind::object(lua, value).push(lua);
	}
	template<class T>
	void PushNumber(lua_State *lua, T t);
	template<class T>
	void PushInt(lua_State *lua, T t);
	template<typename T>
	void PushRaw(lua_State *l, T &&value)
	{
		if constexpr(std::is_pointer_v<T>) {
			luabind::detail::pointer_converter pc;
			pc.to_lua(l, value);
		}
		else {
			luabind::detail::value_converter vc;
			vc.to_lua(l, value);
		}
	}
	DLLLUA void PushBool(lua_State *lua, bool b);
	DLLLUA void PushCFunction(lua_State *lua, lua_CFunction f);
	DLLLUA void PushInt(lua_State *lua, ptrdiff_t i);
	DLLLUA void PushNil(lua_State *lua);
	DLLLUA void PushNumber(lua_State *lua, float f);
	DLLLUA void PushNumber(lua_State *lua, double d);
	DLLLUA void PushString(lua_State *lua, const char *str);
	DLLLUA void PushString(lua_State *lua, const std::string &str);

	template<class T>
	T ToInt(lua_State *lua, int32_t idx);
	template<class T>
	T ToNumber(lua_State *lua, int32_t idx);
	DLLLUA bool ToBool(lua_State *lua, int32_t idx);
	DLLLUA lua_CFunction ToCFunction(lua_State *lua, int32_t idx);
	DLLLUA ptrdiff_t ToInt(lua_State *lua, int32_t idx);
	DLLLUA double ToNumber(lua_State *lua, int32_t idx);
	DLLLUA const char *ToString(lua_State *lua, int32_t idx);
	DLLLUA void *ToUserData(lua_State *lua, int32_t idx);

	template<class T>
	T CheckInt(lua_State *lua, int32_t idx);
	template<class T>
	T CheckNumber(lua_State *lua, int32_t idx);
	DLLLUA ptrdiff_t CheckInt(lua_State *lua, int32_t idx);
	DLLLUA double CheckNumber(lua_State *lua, int32_t idx);
	DLLLUA const char *CheckString(lua_State *lua, int32_t idx);
	DLLLUA void CheckType(lua_State *lua, int32_t narg, int32_t t);
	DLLLUA void CheckTable(lua_State *lua, int32_t narg);
	DLLLUA void *CheckUserData(lua_State *lua, int32_t narg, const std::string &tname);
	DLLLUA void CheckUserData(lua_State *lua, int32_t n);
	DLLLUA void CheckFunction(lua_State *lua, int32_t idx);
	DLLLUA void CheckNil(lua_State *lua, int32_t idx);
	DLLLUA void CheckThread(lua_State *lua, int32_t idx);
	DLLLUA bool CheckBool(lua_State *lua, int32_t idx);
	DLLLUA bool PushLuaFunctionFromString(lua_State *l, const std::string &luaFunction, const std::string &chunkName, std::string &outErrMsg);

	DLLLUA Type GetType(lua_State *lua, int32_t idx);
	DLLLUA const char *GetTypeName(lua_State *lua, int32_t idx);

	DLLLUA void Pop(lua_State *lua, int32_t n = 1);
	// Generates a lua error from the stack top
	DLLLUA void Error(lua_State *lua);
	DLLLUA void Error(lua_State *lua, const std::string &err);
	DLLLUA void Register(lua_State *lua, const char *name, lua_CFunction f);
	DLLLUA void RegisterEnum(lua_State *l, const std::string &name, int32_t val);
    DLLLUA std::shared_ptr<luabind::module_> RegisterLibrary(lua_State *lua, const std::string &name,const std::vector<luaL_Reg>& functions);
	// Creates a reference of the object at the stack top and pops it
	DLLLUA int32_t CreateReference(lua_State *lua, int32_t t = RegistryIndex);
	DLLLUA void ReleaseReference(lua_State *lua, int32_t ref, int32_t t = RegistryIndex);
	DLLLUA void PushRegistryValue(lua_State *l, int32_t n);
	// Moves the top element of the stack to the given index
	DLLLUA void Insert(lua_State *lua, int32_t idx);
	// Grabs the global value with the given name and pushes it onto the stack
	DLLLUA void GetGlobal(lua_State *lua, const std::string &name);
	DLLLUA void SetGlobal(lua_State *lua, const std::string &name);
	template<class T>
	void SetGlobalInt(lua_State *lua, const std::string &name, T i);
	DLLLUA int32_t GetStackTop(lua_State *lua);
	DLLLUA void SetStackTop(lua_State *lua, int32_t idx);

	// Creates a new table and registers there the functions in the list.
	DLLLUA void CreateLibrary(lua_State *l, const luaL_Reg *list);

	DLLLUA int32_t PushTable(lua_State *lua, int32_t n, int32_t idx = RegistryIndex);
	// Inserts a new value into the table at the given index. The value is whatever is at the stack top, the key is whatever is one below. Both are popped from the stack.
	DLLLUA void SetTableValue(lua_State *lua, int32_t idx);
	DLLLUA void GetTableValue(lua_State *lua, int32_t idx);
	DLLLUA StatusCode GetProtectedTableValue(lua_State *lua, int32_t idx);
	// If successful, the value will be the topmost element on the stack
	DLLLUA StatusCode GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key);
	DLLLUA StatusCode GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, std::string &val);
	DLLLUA StatusCode GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, float &val);
	DLLLUA StatusCode GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, int32_t &val);
	DLLLUA StatusCode GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, bool &val);

	// Assigns the key n of a table at the index idx to whatever is at the top of the stack
	DLLLUA void SetTableValue(lua_State *lua, int32_t idx, int32_t n);
	DLLLUA std::size_t GetObjectLength(lua_State *l, int32_t idx);
	DLLLUA std::size_t GetObjectLength(lua_State *l, const luabind::object &o);

	// Pushes a copy of the given element to the top of the stack
	DLLLUA void PushValue(lua_State *lua, int32_t idx);
	// Removes the given element and shifts down all elements after it
	DLLLUA void RemoveValue(lua_State *lua, int32_t idx);
	// Moves the top element on the stack to the given index and shifts up all elements after it
	DLLLUA void InsertValue(lua_State *lua, int32_t idx);

	DLLLUA int32_t SetMetaTable(lua_State *lua, int32_t idx);
	DLLLUA int32_t GetNextPair(lua_State *lua, int32_t idx);

	DLLLUA void CollectGarbage(lua_State *lua);

	DLLLUA StatusCode ExecuteFile(lua_State *lua, std::string &fInOut, int32_t (*traceback)(lua_State *) = nullptr, int32_t numRet = 0);
	DLLLUA StatusCode IncludeFile(lua_State *lua, std::string &fInOut, int32_t (*traceback)(lua_State *) = nullptr, int32_t numRet = 0);
	DLLLUA std::string GetIncludePath();
	DLLLUA std::string GetIncludePath(const std::string &f);
	DLLLUA StatusCode RunString(lua_State *lua, const std::string &str, int32_t retCount, const std::string &chunkName, int32_t (*traceback)(lua_State *) = nullptr);
	DLLLUA StatusCode RunString(lua_State *lua, const std::string &str, const std::string &chunkName, int32_t (*traceback)(lua_State *) = nullptr);
	DLLLUA void ExecuteFiles(lua_State *lua, const std::string &subPath, int32_t (*traceback)(lua_State *) = nullptr, const std::function<void(StatusCode, const std::string &)> &fCallback = nullptr);
	DLLLUA const char *GetTypeString(lua_State *l, int32_t n);
	DLLLUA luabind::weak_ref CreateWeakReference(const luabind::object &o);
	DLLLUA void PushWeakReference(const luabind::weak_ref &ref);
	DLLLUA luabind::object WeakReferenceToObject(const luabind::weak_ref &ref);
	DLLLUA void RegisterLibraryEnums(lua_State *l, const std::string &libName, const std::unordered_map<std::string, lua_Integer> &enums);
	template<typename T>
	void RegisterLibraryValues(lua_State *l, const std::string &libName, const std::unordered_map<std::string, T> &values);

	template<class T>
	void RegisterLibraryValue(lua_State *l, const std::string &libName, const std::string &key, const T &val);

	DLLLUA void GetField(lua_State *l, int32_t idx, const std::string &fieldName);

	DLLLUA void set_precompiled_files_enabled(bool bEnabled);
	DLLLUA bool are_precompiled_files_enabled();

	DLLLUA void get_global_nested_library(lua_State *l, const std::string &name);

	// Expects a lua function at the top of the stack
	DLLLUA bool compile_file(lua_State *l, const std::string &outPath);
	DLLLUA std::string get_current_file(lua_State *l);
};

template<typename T>
void Lua::RegisterLibraryValues(lua_State *l, const std::string &libName, const std::unordered_map<std::string, T> &values)
{
	get_global_nested_library(l, libName);
	if(Lua::IsNil(l, -1))
		throw std::runtime_error("No library '" + libName + " found!");
	auto t = GetStackTop(l);
	if(!IsNil(l, t)) {
		for(auto &pair : values) {
			PushString(l, pair.first);
			Push<T>(l, pair.second);
			SetTableValue(l, t);
		}
	}
	Pop(l, 1);
}

template<class T>
T Lua::CheckInt(lua_State *lua, int32_t idx)
{
	return static_cast<T>(CheckInt(lua, idx));
}
template<class T>
T Lua::CheckNumber(lua_State *lua, int32_t idx)
{
	return static_cast<T>(CheckNumber(lua, idx));
}

template<class T>
T Lua::ToInt(lua_State *lua, int32_t idx)
{
	return static_cast<T>(ToInt(lua, idx));
}
template<class T>
T Lua::ToNumber(lua_State *lua, int32_t idx)
{
	return static_cast<T>(CheckNumber(lua, idx));
}

template<class T>
void Lua::PushNumber(lua_State *lua, T t)
{
	PushNumber(lua, static_cast<double>(t));
}
template<class T>
void Lua::PushInt(lua_State *lua, T t)
{
	PushInt(lua, static_cast<ptrdiff_t>(t));
}

template<class T>
void Lua::SetGlobalInt(lua_State *lua, const std::string &name, T i)
{
	PushInt(lua, i);
	SetGlobal(lua, name);
}

template<class T>
void Lua::RegisterLibraryValue(lua_State *l, const std::string &libName, const std::string &key, const T &val)
{
	GetGlobal(l, libName);
	auto t = GetStackTop(l);
	if(!IsNil(l, t)) {
		PushString(l, key);
		Push<T>(l, val);
		SetTableValue(l, t);
	}
	Pop(l, 1);
}

#endif
