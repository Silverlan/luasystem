// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "lua_headers.hpp"

export module pragma.lua:core;

export import pragma.filesystem;

export import luabind;

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

export {
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

		CONSTEXPR_DLL_COMPAT std::string SCRIPT_DIRECTORY = "lua";
		CONSTEXPR_DLL_COMPAT std::string SCRIPT_DIRECTORY_SLASH = "lua/";
		CONSTEXPR_DLL_COMPAT std::string FILE_EXTENSION = "lua";
		CONSTEXPR_DLL_COMPAT std::string FILE_EXTENSION_PRECOMPILED = "luac";
		CONSTEXPR_DLL_COMPAT std::string DOT_FILE_EXTENSION = ".lua";
		CONSTEXPR_DLL_COMPAT std::string DOT_FILE_EXTENSION_PRECOMPILED = ".luac";

		template<typename T>
		using base_type = typename std::remove_cv_t<std::remove_pointer_t<std::remove_reference_t<T>>>;
		template<typename T>
		concept is_native_type = std::is_arithmetic_v<T> || std::is_same_v<base_type<T>, std::string> || std::is_same_v<T, const char *>;

		const auto RegistryIndex = LUA_REGISTRYINDEX;
		DLLLUA lua_State *CreateState();
		DLLLUA void CloseState(lua_State *lua);
		DLLLUA void Call(lua_State *lua, int32_t nargs, int32_t nresults);
		// Function and function arguments have to be pushed inside 'pushFuncArgs'-callback!
		DLLLUA StatusCode ProtectedCall(lua_State *lua, const std::function<StatusCode(lua_State *)> &pushFuncArgs, int32_t numResults, std::string &outErr, int32_t (*traceback)(lua_State *) = nullptr, void (*pushArgErrorHandler)(lua_State *, StatusCode) = nullptr);
		DLLLUA StatusCode ProtectedCall(lua_State *lua, int32_t nargs, int32_t nresults, std::string &outErr, int32_t (*traceback)(lua_State *) = nullptr, void (*pushArgErrorHandler)(lua_State *, StatusCode) = nullptr);
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
		DLLLUA std::shared_ptr<luabind::module_> RegisterLibrary(lua_State *lua, const std::string &name, const std::vector<luaL_Reg> &functions);
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
		DLLLUA void SetRaw(lua_State *lua, int32_t idx);
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

		DLLLUA StatusCode ExecuteFile(lua_State *lua, std::string &fInOut, std::string &outErr, int32_t (*traceback)(lua_State *) = nullptr, int32_t numRet = 0, void (*loadErrorHandler)(lua_State *, StatusCode) = nullptr);
		DLLLUA StatusCode IncludeFile(lua_State *lua, std::string &fInOut, std::string &outErr, int32_t (*traceback)(lua_State *) = nullptr, int32_t numRet = 0, void (*loadErrorHandler)(lua_State *, StatusCode) = nullptr);
		DLLLUA std::string GetIncludePath();
		DLLLUA std::string GetIncludePath(const std::string &f);
		DLLLUA StatusCode RunString(lua_State *lua, const std::string &str, int32_t retCount, const std::string &chunkName, std::string &outErr, int32_t (*traceback)(lua_State *) = nullptr, void (*loadErrorHandler)(lua_State *, StatusCode) = nullptr);
		DLLLUA StatusCode RunString(lua_State *lua, const std::string &str, const std::string &chunkName, std::string &outErr, int32_t (*traceback)(lua_State *) = nullptr, void (*loadErrorHandler)(lua_State *, StatusCode) = nullptr);
		DLLLUA void ExecuteFiles(lua_State *lua, const std::string &subPath, std::string &outErr, int32_t (*traceback)(lua_State *) = nullptr, const std::function<void(StatusCode, const std::string &)> &fCallback = nullptr);
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

		DLLLUA StatusCode LoadFile(lua_State *lua, std::string &fInOut, fsys::SearchFlags includeFlags = fsys::SearchFlags::All, fsys::SearchFlags excludeFlags = fsys::SearchFlags::None);
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
}

export namespace lua {
	using Integer = lua_Integer;
	using Number = lua_Number;

	DLLLUA int snapshot(lua_State *l);

	enum class Type : int {
		None = LUA_TNONE,

		Nil = LUA_TNIL,
		Boolean = LUA_TBOOLEAN,
		LightUserData = LUA_TLIGHTUSERDATA,
		Number = LUA_TNUMBER,
		String = LUA_TTABLE,
		Table = LUA_TTABLE,
		Function = LUA_TFUNCTION,
		UserData = LUA_TUSERDATA,
		Thread = LUA_TTHREAD,
	};

	inline lua_State *new_state(lua_Alloc f, void *ud) { return lua_newstate(f, ud); }
	inline void close(lua_State *L) { lua_close(L); }
	inline lua_State *new_thread(lua_State *L) { return lua_newthread(L); }

	inline lua_CFunction at_panic(lua_State *L, lua_CFunction panicf) { return lua_atpanic(L, panicf); }

	inline int get_top(lua_State *L) { return lua_gettop(L); }
	inline void set_top(lua_State *L, int idx) { lua_settop(L, idx); }
	inline void push_value(lua_State *L, int idx) { lua_pushvalue(L, idx); }
	inline void remove(lua_State *L, int idx) { lua_remove(L, idx); }
	inline void insert(lua_State *L, int idx) { lua_insert(L, idx); }
	inline void replace(lua_State *L, int idx) { lua_replace(L, idx); }
	inline int check_stack(lua_State *L, int sz) { return lua_checkstack(L, sz); }

	inline void move_between_states(lua_State *from, lua_State *to, int n) { lua_xmove(from, to, n); }

	inline int is_number(lua_State *L, int idx) { return lua_isnumber(L, idx); }
	inline int is_string(lua_State *L, int idx) { return lua_isstring(L, idx); }
	inline int is_c_function(lua_State *L, int idx) { return lua_iscfunction(L, idx); }
	inline int is_user_data(lua_State *L, int idx) { return lua_isuserdata(L, idx); }
	inline Type type(lua_State *L, int idx) { return static_cast<Type>(lua_type(L, idx)); }
	inline const char *type_name(lua_State *L, int tp) { return lua_typename(L, tp); }

	inline int equal(lua_State *L, int idx1, int idx2) { return lua_equal(L, idx1, idx2); }
	inline int raw_equal(lua_State *L, int idx1, int idx2) { return lua_rawequal(L, idx1, idx2); }
	inline int less_than(lua_State *L, int idx1, int idx2) { return lua_lessthan(L, idx1, idx2); }

	inline lua_Number to_number(lua_State *L, int idx) { return lua_tonumber(L, idx); }
	inline lua_Integer to_integer(lua_State *L, int idx) { return lua_tointeger(L, idx); }
	inline int to_boolean(lua_State *L, int idx) { return lua_toboolean(L, idx); }
	inline const char *to_string(lua_State *L, int idx, size_t *len) { return lua_tolstring(L, idx, len); }
	inline size_t obj_len(lua_State *L, int idx) { return lua_objlen(L, idx); }
	inline lua_CFunction to_c_function(lua_State *L, int idx) { return lua_tocfunction(L, idx); }
	inline void *to_user_data(lua_State *L, int idx) { return lua_touserdata(L, idx); }
	inline lua_State *to_thread(lua_State *L, int idx) { return lua_tothread(L, idx); }
	inline const void *to_pointer(lua_State *L, int idx) { return lua_topointer(L, idx); }

	inline void push_nil(lua_State *L) { lua_pushnil(L); }
	inline void push_number(lua_State *L, lua_Number n) { lua_pushnumber(L, n); }
	inline void push_integer(lua_State *L, lua_Integer n) { lua_pushinteger(L, n); }
	inline void push_string(lua_State *L, const char *s, size_t l) { lua_pushlstring(L, s, l); }
	inline void push_string(lua_State *L, const char *s) { lua_pushstring(L, s); }
	inline const char *push_formatted_vstring(lua_State *L, const char *fmt, va_list argp) { return lua_pushvfstring(L, fmt, argp); }
	inline const char *push_formatted_string(lua_State *L, const char *fmt, ...)
	{
		va_list ap;
		va_start(ap, fmt);
		const char *res = lua_pushvfstring(L, fmt, ap);
		va_end(ap);
		return res;
	}
	inline void push_c_closure(lua_State *L, lua_CFunction fn, int n) { lua_pushcclosure(L, fn, n); }
	inline void push_boolean(lua_State *L, int b) { lua_pushboolean(L, b); }
	inline void push_light_user_data(lua_State *L, void *p) { lua_pushlightuserdata(L, p); }
	inline int push_thread(lua_State *L) { return lua_pushthread(L); }

	inline void get_table(lua_State *L, int idx) { lua_gettable(L, idx); }
	inline void get_field(lua_State *L, int idx, const char *k) { lua_getfield(L, idx, k); }
	inline void raw_get(lua_State *L, int idx) { lua_rawget(L, idx); }
	inline void raw_get(lua_State *L, int idx, int n) { lua_rawgeti(L, idx, n); }
	inline void create_table(lua_State *L, int narr, int nrec) { lua_createtable(L, narr, nrec); }
	inline void *new_user_data(lua_State *L, size_t sz) { return lua_newuserdata(L, sz); }
	inline int get_meta_table(lua_State *L, int objindex) { return lua_getmetatable(L, objindex); }
	inline void get_function_env(lua_State *L, int idx) { lua_getfenv(L, idx); }

	inline void set_table(lua_State *L, int idx) { lua_settable(L, idx); }
	inline void set_field(lua_State *L, int idx, const char *k) { lua_setfield(L, idx, k); }
	inline void raw_set(lua_State *L, int idx) { lua_rawset(L, idx); }
	inline void raw_set(lua_State *L, int idx, int n) { lua_rawseti(L, idx, n); }
	inline int set_meta_table(lua_State *L, int objindex) { return lua_setmetatable(L, objindex); }
	inline int set_function_env(lua_State *L, int idx) { return lua_setfenv(L, idx); }

	inline void call(lua_State *L, int nargs, int nresults) { lua_call(L, nargs, nresults); }
	inline int pcall(lua_State *L, int nargs, int nresults, int errfunc) { return lua_pcall(L, nargs, nresults, errfunc); }
	inline int cpcall(lua_State *L, lua_CFunction func, void *ud) { return lua_cpcall(L, func, ud); }
	inline int load(lua_State *L, lua_Reader reader, void *dt, const char *chunkname) { return lua_load(L, reader, dt, chunkname); }

	inline int dump(lua_State *L, lua_Writer writer, void *data) { return lua_dump(L, writer, data); }
	inline int dump_strip(lua_State *L, lua_Writer writer, void *data, int strip) { return lua_dump_strip(L, writer, data, strip); }

	inline int yield(lua_State *L, int nresults) { return lua_yield(L, nresults); }
	inline int resume(lua_State *L, int narg) { return lua_resume(L, narg); }
	inline int status(lua_State *L) { return lua_status(L); }

	inline int gc(lua_State *L, int what, int data) { return lua_gc(L, what, data); }

	inline int error(lua_State *L) { return lua_error(L); }
	inline int error(lua_State *L, int arg, const char *extramsg) { return luaL_argerror(L, arg, extramsg); }
	inline int next(lua_State *L, int idx) { return lua_next(L, idx); }
	inline void concat(lua_State *L, int n) { lua_concat(L, n); }

	inline lua_Alloc get_alloc_function(lua_State *L, void **ud) { return lua_getallocf(L, ud); }
	inline void set_alloc_function(lua_State *L, lua_Alloc f, void *ud) { lua_setallocf(L, f, ud); }

	inline void set_level(lua_State *from, lua_State *to) { lua_setlevel(from, to); }

	using hook = lua_Hook;

	inline int get_stack(lua_State *L, int level, lua_Debug *ar) { return lua_getstack(L, level, ar); }
	inline int get_info(lua_State *L, const char *what, lua_Debug *ar) { return lua_getinfo(L, what, ar); }
	inline const char *get_local(lua_State *L, const lua_Debug *ar, int n) { return lua_getlocal(L, ar, n); }
	inline const char *set_local(lua_State *L, const lua_Debug *ar, int n) { return lua_setlocal(L, ar, n); }
	inline const char *get_upvalue(lua_State *L, int funcindex, int n) { return lua_getupvalue(L, funcindex, n); }
	inline const char *set_upvalue(lua_State *L, int funcindex, int n) { return lua_setupvalue(L, funcindex, n); }
	inline int set_hook(lua_State *L, lua_Hook func, int mask, int count) { return lua_sethook(L, func, mask, count); }
	inline lua_Hook get_hook(lua_State *L) { return lua_gethook(L); }
	inline int get_hook_mask(lua_State *L) { return lua_gethookmask(L); }
	inline int get_hook_count(lua_State *L) { return lua_gethookcount(L); }

	inline void *upvalue_id(lua_State *L, int idx, int n) { return lua_upvalueid(L, idx, n); }
	inline void join_upvalues(lua_State *L, int idx1, int n1, int idx2, int n2) { lua_upvaluejoin(L, idx1, n1, idx2, n2); }
	inline int load_with_mode(lua_State *L, lua_Reader reader, void *dt, const char *chunkname, const char *mode) { return lua_loadx(L, reader, dt, chunkname, mode); }
	inline const lua_Number *version(lua_State *L) { return lua_version(L); }
	inline void copy_stack_value(lua_State *L, int fromidx, int toidx) { lua_copy(L, fromidx, toidx); }
	inline lua_Number tonumber_checked(lua_State *L, int idx, int *isnum) { return lua_tonumberx(L, idx, isnum); }
	inline lua_Integer tointeger_checked(lua_State *L, int idx, int *isnum) { return lua_tointegerx(L, idx, isnum); }

	inline int is_yieldable(lua_State *L) { return lua_isyieldable(L); }

	inline int open_base(lua_State *L) { return luaopen_base(L); }
	inline int open_math(lua_State *L) { return luaopen_math(L); }
	inline int open_string(lua_State *L) { return luaopen_string(L); }
	inline int open_table(lua_State *L) { return luaopen_table(L); }
	inline int open_io(lua_State *L) { return luaopen_io(L); }
	inline int open_os(lua_State *L) { return luaopen_os(L); }
	inline int open_package(lua_State *L) { return luaopen_package(L); }
	inline int open_debug(lua_State *L) { return luaopen_debug(L); }
	inline int open_bit(lua_State *L) { return luaopen_bit(L); }
	inline int open_jit(lua_State *L) { return luaopen_jit(L); }
	inline int open_ffi(lua_State *L) { return luaopen_ffi(L); }
	inline int open_string_buffer(lua_State *L) { return luaopen_string_buffer(L); }

	CONSTEXPR_COMPAT const char *LIB_COROUTINE = LUA_COLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_MATH = LUA_MATHLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_STRING = LUA_STRLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_TABLE = LUA_TABLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_IO = LUA_IOLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_OS = LUA_OSLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_PACKAGE = LUA_LOADLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_DEBUG = LUA_DBLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_BIT = LUA_BITLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_JIT = LUA_JITLIBNAME;
	CONSTEXPR_COMPAT const char *LIB_FFI = LUA_FFILIBNAME;

#ifndef USE_LUAJIT
	CONSTEXPR_COMPAT const char *LIB_UTF8 = LUA_UTF8LIBNAME inline int open_utf8(lua_State * L) { return luaopen_utf8(L); }
	inline int open_coroutine(lua_State *L) { return luaopen_coroutine(L); }
#endif
#if defined(LUA_COMPAT_BITLIB)
	inline int open_bit32(lua_State *L) { return luaopen_bit32(L); }
#endif

	inline void open_libs(lua_State *L) { luaL_openlibs(L); }

	inline void open_lib(lua_State *L, const char *libname, const luaL_Reg *l, int nup) { luaL_openlib(L, libname, l, nup); }
	inline void register_functions(lua_State *L, const char *libname, const luaL_Reg *l) { luaL_register(L, libname, l); }
	inline int get_meta_field(lua_State *L, int obj, const char *e) { return luaL_getmetafield(L, obj, e); }
	inline int call_meta(lua_State *L, int obj, const char *e) { return luaL_callmeta(L, obj, e); }
	inline int type_error(lua_State *L, int narg, const char *tname) { return luaL_typerror(L, narg, tname); }
	inline int arg_error(lua_State *L, int numarg, const char *extramsg) { return luaL_argerror(L, numarg, extramsg); }
	inline const char *check_string(lua_State *L, int numArg, size_t *l) { return luaL_checklstring(L, numArg, l); }
	inline const char *check_string(lua_State *L, int numArg, const char *def, size_t *l) { return luaL_optlstring(L, numArg, def, l); }
	inline lua_Number check_number(lua_State *L, int numArg) { return luaL_checknumber(L, numArg); }
	inline lua_Number check_number(lua_State *L, int nArg, lua_Number def) { return luaL_optnumber(L, nArg, def); }
	inline lua_Integer check_integer(lua_State *L, int numArg) { return luaL_checkinteger(L, numArg); }
	inline lua_Integer check_integer(lua_State *L, int nArg, lua_Integer def) { return luaL_optinteger(L, nArg, def); }

	inline void check_stack(lua_State *L, int sz, const char *msg) { luaL_checkstack(L, sz, msg); }
	inline void check_type(lua_State *L, int narg, int t) { luaL_checktype(L, narg, t); }
	inline void check_any(lua_State *L, int narg) { luaL_checkany(L, narg); }

	inline int new_meta_table(lua_State *L, const char *tname) { return luaL_newmetatable(L, tname); }
	inline void *check_user_data(lua_State *L, int ud, const char *tname) { return luaL_checkudata(L, ud, tname); }

	inline void where(lua_State *L, int lvl) { luaL_where(L, lvl); }

	inline int check_option(lua_State *L, int narg, const char *def, const char *const lst[]) { return luaL_checkoption(L, narg, def, lst); }

	inline int ref(lua_State *L, int t) { return luaL_ref(L, t); }
	inline void unref(lua_State *L, int t, int ref) { luaL_unref(L, t, ref); }

	inline int load_file(lua_State *L, const char *filename) { return luaL_loadfile(L, filename); }
	inline int load_buffer(lua_State *L, const char *buff, size_t sz, const char *name) { return luaL_loadbuffer(L, buff, sz, name); }
	inline int load_string(lua_State *L, const char *s) { return luaL_loadstring(L, s); }

	inline lua_State *new_state() { return luaL_newstate(); }

	inline const char *replace_substrings(lua_State *L, const char *s, const char *p, const char *r) { return luaL_gsub(L, s, p, r); }
	inline const char *find_table(lua_State *L, int idx, const char *fname, int szhint) { return luaL_findtable(L, idx, fname, szhint); }

	inline int file_result(lua_State *L, int stat, const char *fname) { return luaL_fileresult(L, stat, fname); }
	inline int exec_result(lua_State *L, int stat) { return luaL_execresult(L, stat); }
	inline int load_file(lua_State *L, const char *filename, const char *mode) { return luaL_loadfilex(L, filename, mode); }
	inline int load_buffer(lua_State *L, const char *buff, size_t sz, const char *name, const char *mode) { return luaL_loadbufferx(L, buff, sz, name, mode); }
	inline void trace_back(lua_State *L, lua_State *L1, const char *msg, int level) { luaL_traceback(L, L1, msg, level); }
	inline void set_functions(lua_State *L, const luaL_Reg *l, int nup) { luaL_setfuncs(L, l, nup); }
	inline void push_module(lua_State *L, const char *modname, int sizehint) { luaL_pushmodule(L, modname, sizehint); }
	inline void *test_user_data(lua_State *L, int ud, const char *tname) { return luaL_testudata(L, ud, tname); }
	inline void set_meta_table(lua_State *L, const char *tname) { luaL_setmetatable(L, tname); }

	inline void buffer_init(lua_State *L, luaL_Buffer *B) { luaL_buffinit(L, B); }
	inline char *buffer_prepare(luaL_Buffer *B) { return luaL_prepbuffer(B); }
	inline void buffer_add_string(luaL_Buffer *B, const char *s, size_t l) { luaL_addlstring(B, s, l); }
	inline void buffer_add_string(luaL_Buffer *B, const char *s) { luaL_addstring(B, s); }
	inline void buffer_add_value(luaL_Buffer *B) { luaL_addvalue(B); }
	inline void buffer_push_result(luaL_Buffer *B) { luaL_pushresult(B); }

	inline void pop(lua_State *L, int n) { set_top(L, -(n)-1); }
	inline void new_table(lua_State *L) { create_table(L, 0, 0); }

	template<std::size_t N>
	void push_literal(lua_State *L, const char (&s)[N])
	{
		push_string(L, s, N - 1);
	}

	inline void push_c_function(lua_State *L, lua_CFunction f) { push_c_closure(L, f, 0); }
	inline size_t str_len(lua_State *L, int i) { return obj_len(L, i); }

	inline bool is_function(lua_State *L, int n) { return type(L, n) == Type::Function; }
	inline bool is_table(lua_State *L, int n) { return type(L, n) == Type::Table; }
	inline bool is_light_user_data(lua_State *L, int n) { return type(L, n) == Type::LightUserData; }
	inline bool is_nil(lua_State *L, int n) { return type(L, n) == Type::Nil; }
	inline bool is_boolean(lua_State *L, int n) { return type(L, n) == Type::Boolean; }
	inline bool is_thread(lua_State *L, int n) { return type(L, n) == Type::Thread; }
	inline bool is_none(lua_State *L, int n) { return type(L, n) == Type::None; }
	inline bool is_none_or_nil(lua_State *L, int n) { return static_cast<int>(type(L, n)) <= 0; }

	inline void set_global(lua_State *L, const char *s) { set_field(L, LUA_GLOBALSINDEX, s); }
	inline void get_global(lua_State *L, const char *s) { get_field(L, LUA_GLOBALSINDEX, s); }
	inline const char *to_string(lua_State *L, int idx) { return to_string(L, idx, nullptr); }

	inline void register_function(lua_State *L, const char *n, lua_CFunction f)
	{
		push_c_function(L, f);
		set_global(L, n);
	}

	inline int create_reference(lua_State *l, int index)
	{
		lua_pushvalue(l, index);
		return luaL_ref(l, LUA_REGISTRYINDEX);
	}

	inline void remove_reference(lua_State *l, int index) { luaL_unref(l, LUA_REGISTRYINDEX, index); }

	inline const char *get_type(lua_State *l, int n)
	{
		const char *arg = lua_tostring(l, n);
		if(arg == NULL) {
			int type = lua_type(l, n);
			switch(type) {
			case LUA_TNIL:
				{
					arg = "nil";
					break;
				}
			case LUA_TBOOLEAN:
				{
					arg = "Boolean";
					break;
				}

			case LUA_TLIGHTUSERDATA:
				{
					arg = "LightUserData";
					break;
				}
			case LUA_TNUMBER:
				{
					arg = "Number";
					break;
				}
			case LUA_TSTRING:
				{
					arg = "String";
					break;
				}
			case LUA_TTABLE:
				{
					arg = "Table";
					break;
				}
			case LUA_TFUNCTION:
				{
					arg = "Function";
					break;
				}
			case LUA_TUSERDATA:
				{
					arg = "UserData";
					break;
				}
			case LUA_TTHREAD:
				{
					arg = "Thread";
					break;
				}
			default:
				arg = "Unknown";
			}
		}
		return arg;
	};
	using State = lua_State;
}
