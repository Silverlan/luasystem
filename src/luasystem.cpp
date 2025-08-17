// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "luasystem_file.h"
#include <fsys/filesystem.h>
#include "impl_luajit_definitions.hpp"
#include <sharedutils/util_string.h>

static void get_file_chunk_name(std::string &fileName)
{
	ustring::replace(fileName, "\\", "/");
	fileName = "@" + fileName;
}
static std::string get_file_chunk_name(const std::string &fileName)
{
	auto chunkName = fileName;
	get_file_chunk_name(chunkName);
	return chunkName;
}

lua_State *Lua::CreateState()
{
	lua_State *lua = luaL_newstate();
	luaopen_io(lua);
	luaopen_table(lua);
	luaopen_string(lua);
	luaopen_math(lua);
	luaL_openlibs(lua);
	return lua;
}

void Lua::CloseState(lua_State *lua) { lua_close(lua); }

static bool s_precompiledFilesEnabled = true;
void Lua::set_precompiled_files_enabled(bool bEnabled) { s_precompiledFilesEnabled = bEnabled; }
bool Lua::are_precompiled_files_enabled() { return s_precompiledFilesEnabled; }

void Lua::get_global_nested_library(lua_State *l, const std::string &name)
{
	std::vector<std::string> libs;
	ustring::explode(name, ".", libs);
	if(libs.empty() == false) {
		Lua::GetGlobal(l, libs.front()); /* 1 */
		if(Lua::IsSet(l, -1) == false) {
			Lua::Pop(l, 1); /* 0 */
			Lua::PushNil(l);
			return;
		}
		if(libs.size() == 1)
			return;
		for(auto it = libs.begin() + 1; it != libs.end(); ++it) {
			auto bLast = (it == libs.end() - 1);
			auto &lib = *it;
			auto t = Lua::GetStackTop(l);
			auto status = Lua::GetProtectedTableValue(l, t, lib);
			if(status != Lua::StatusCode::Ok) {
				Lua::PushNil(l);
				return;
			}
			Lua::RemoveValue(l, -2);
			if(bLast)
				return;
		}
	}
	Lua::PushNil(l);
}

Lua::StatusCode Lua::LoadFile(lua_State *lua, std::string &fInOut, fsys::SearchFlags includeFlags, fsys::SearchFlags excludeFlags)
{
	if(s_precompiledFilesEnabled) {
		if(fInOut.length() > 3 && fInOut.substr(fInOut.length() - 4) == DOT_FILE_EXTENSION) {
			auto cpath = fInOut.substr(0, fInOut.length() - 4) + DOT_FILE_EXTENSION_PRECOMPILED;
			if(FileManager::Exists(SCRIPT_DIRECTORY_SLASH + cpath, includeFlags, excludeFlags) == true)
				fInOut = cpath;
		}
	}
	fInOut = FileManager::GetNormalizedPath(SCRIPT_DIRECTORY_SLASH + fInOut);
	std::string err;
	auto f = FileManager::OpenFile(fInOut.c_str(), "rb", &err, includeFlags, excludeFlags);
	if(f == nullptr) {
		lua_pushfstring(lua, ("cannot open %s: " + (!err.empty() ? err : "File not found")).c_str(), fInOut.c_str());
		return StatusCode::ErrorFile;
	}
	auto nf = fInOut;
	auto fReal = std::dynamic_pointer_cast<VFilePtrInternalReal>(f);
	if(fReal != nullptr) {
		// We need the full path (at least relative to the program), otherwise the ZeroBrane debugger may not work in some cases
		nf = FileManager::GetCanonicalizedPath(fReal->GetPath());
		static auto len = FileManager::GetProgramPath().length();
		nf = nf.substr(len + 1);
	}
	auto l = f->GetSize();
	std::vector<char> buf(l);
	f->Read(buf.data(), l);

	get_file_chunk_name(nf);
	return static_cast<StatusCode>(luaL_loadbuffer(lua, buf.data(), l, nf.c_str()));
}

void Lua::Call(lua_State *lua, int32_t nargs, int32_t nresults) { lua_call(lua, nargs, nresults); }

static Lua::StatusCode protected_call(lua_State *lua, const std::function<Lua::StatusCode(lua_State *)> &pushArgs, int32_t numArgs, int32_t numResults, std::string &outErr, int32_t (*traceback)(lua_State *), void (*pushArgErrorHandler)(lua_State *, Lua::StatusCode))
{
	int32_t tracebackIdx = 0;
	if(traceback != nullptr) {
		tracebackIdx = Lua::GetStackTop(lua) - numArgs + 1;
		if(!pushArgs) {
			// In this case the function has already been pushed onto the stack and we have to
			// push the traceback function before it
			--tracebackIdx;
		}
		lua_pushcfunction(lua, traceback);
		lua_insert(lua, tracebackIdx);
	}
	auto top = Lua::GetStackTop(lua);
	int32_t s = 0;
	if(pushArgs != nullptr) {
		auto pushArgStatusCode = pushArgs(lua);
		s = umath::to_integral(pushArgStatusCode);
		if(s != 0) {
			if(traceback || pushArgErrorHandler) {
				Lua::RemoveValue(lua, tracebackIdx);
				// pushArgErrorHandler can be used if argument errors should be handled differently
				if(pushArgErrorHandler)
					pushArgErrorHandler(lua, pushArgStatusCode);
				else
					traceback(lua);
			}
			auto newTop = Lua::GetStackTop(lua);
			if(newTop > top)
				Lua::Pop(lua, newTop - top);

			outErr = Lua::CheckString(lua, -1);
			Lua::Pop(lua);
			return static_cast<Lua::StatusCode>(s);
		}
		// pushArgs pushes both the arguments AND the function itself, so
		// we need to reduce numArgs by 1 to account for the function.
		--numArgs;
	}

	numArgs += Lua::GetStackTop(lua) - top;
	auto r = lua_pcall(lua, numArgs, numResults, tracebackIdx);
	if(traceback != nullptr)
		Lua::RemoveValue(lua, tracebackIdx);
	auto statusCode = static_cast<Lua::StatusCode>(r);
	if(statusCode != Lua::StatusCode::Ok) {
		outErr = Lua::CheckString(lua, -1);
		Lua::Pop(lua);
	}
	return statusCode;
}

Lua::StatusCode Lua::ProtectedCall(lua_State *lua, const std::function<StatusCode(lua_State *)> &pushArgs, int32_t numResults, std::string &outErr, int32_t (*traceback)(lua_State *), void (*pushArgErrorHandler)(lua_State *, StatusCode))
{
	return protected_call(lua, pushArgs, 0, numResults, outErr, traceback, pushArgErrorHandler);
}
Lua::StatusCode Lua::ProtectedCall(lua_State *lua, int32_t nargs, int32_t nresults, std::string &outErr, int32_t (*traceback)(lua_State *), void (*pushArgErrorHandler)(lua_State *, StatusCode))
{
	return protected_call(lua, nullptr, nargs, nresults, outErr, traceback, pushArgErrorHandler);
}

int32_t Lua::CreateTable(lua_State *lua)
{
	lua_newtable(lua);
	return Lua::GetStackTop(lua);
}
int32_t Lua::CreateMetaTable(lua_State *lua, const std::string &tname) { return luaL_newmetatable(lua, tname.c_str()); }

bool Lua::IsBool(lua_State *lua, int32_t idx) { return lua_isboolean(lua, idx); }
bool Lua::IsCFunction(lua_State *lua, int32_t idx) { return (lua_iscfunction(lua, idx) == 1) ? true : false; }
bool Lua::IsFunction(lua_State *lua, int32_t idx) { return lua_isfunction(lua, idx); }
bool Lua::IsNil(lua_State *lua, int32_t idx) { return lua_isnil(lua, idx); }
bool Lua::IsNone(lua_State *lua, int32_t idx) { return lua_isnone(lua, idx); }
bool Lua::IsSet(lua_State *lua, int32_t idx) { return !lua_isnoneornil(lua, idx); }
bool Lua::IsNumber(lua_State *lua, int32_t idx) { return (lua_isnumber(lua, idx) == 1) ? true : false; }
bool Lua::IsString(lua_State *lua, int32_t idx) { return (lua_isstring(lua, idx) == 1) ? true : false; }
bool Lua::IsTable(lua_State *lua, int32_t idx) { return lua_istable(lua, idx); }
bool Lua::IsUserData(lua_State *lua, int32_t idx) { return (lua_isuserdata(lua, idx) == 1) ? true : false; }

void Lua::PushBool(lua_State *lua, bool b) { lua_pushboolean(lua, b); }
void Lua::PushCFunction(lua_State *lua, lua_CFunction f) { lua_pushcfunction(lua, f); }
void Lua::PushInt(lua_State *lua, ptrdiff_t i) { lua_pushinteger(lua, i); }
void Lua::PushNil(lua_State *lua) { lua_pushnil(lua); }
void Lua::PushNumber(lua_State *lua, float f) { lua_pushnumber(lua, f); }
void Lua::PushNumber(lua_State *lua, double d) { lua_pushnumber(lua, d); }
void Lua::PushString(lua_State *lua, const char *str) { lua_pushstring(lua, str); }
void Lua::PushString(lua_State *lua, const std::string &str) { lua_pushstring(lua, str.c_str()); }

bool Lua::ToBool(lua_State *lua, int32_t idx) { return (lua_toboolean(lua, idx) == 1) ? true : false; }
lua_CFunction Lua::ToCFunction(lua_State *lua, int32_t idx) { return lua_tocfunction(lua, idx); }
ptrdiff_t Lua::ToInt(lua_State *lua, int32_t idx) { return lua_tointeger(lua, idx); }
double Lua::ToNumber(lua_State *lua, int32_t idx) { return lua_tonumber(lua, idx); }
const char *Lua::ToString(lua_State *lua, int32_t idx) { return lua_tostring(lua, idx); }
void *Lua::ToUserData(lua_State *lua, int32_t idx) { return lua_touserdata(lua, idx); }

ptrdiff_t Lua::CheckInt(lua_State *lua, int32_t idx)
{
	Lua::CheckNumber(lua, idx);
	return lua_tointeger(lua, idx);
}
double Lua::CheckNumber(lua_State *lua, int32_t idx) { return luaL_checknumber(lua, idx); }
const char *Lua::CheckString(lua_State *lua, int32_t idx) { return luaL_checkstring(lua, idx); }
void Lua::CheckType(lua_State *lua, int32_t narg, int32_t t) { luaL_checktype(lua, narg, t); }
void Lua::CheckTable(lua_State *lua, int32_t narg) { CheckType(lua, narg, LUA_TTABLE); }
void *Lua::CheckUserData(lua_State *lua, int32_t narg, const std::string &tname) { return luaL_checkudata(lua, narg, tname.c_str()); }
void Lua::CheckUserData(lua_State *lua, int32_t n) { return luaL_checkuserdata(lua, n); }
void Lua::CheckFunction(lua_State *lua, int32_t idx) { luaL_checktype(lua, (idx), LUA_TFUNCTION); }
void Lua::CheckNil(lua_State *lua, int32_t idx) { luaL_checktype(lua, (idx), LUA_TNIL); }
void Lua::CheckThread(lua_State *lua, int32_t idx) { luaL_checktype(lua, (idx), LUA_TTHREAD); }
bool Lua::CheckBool(lua_State *lua, int32_t idx)
{
	if(!lua_isboolean(lua, idx)) {
		auto *msg = lua_pushfstring(lua, "%s expected, got %s", lua_typename(lua, LUA_TBOOLEAN), lua_typename(lua, lua_type(lua, idx)));
		luaL_argerror(lua, idx, msg);
	}
	return (lua_toboolean(lua, idx) == 1) ? true : false;
}
bool Lua::PushLuaFunctionFromString(lua_State *l, const std::string &luaFunction, const std::string &chunkName, std::string &outErrMsg)
{
	auto bufLuaFunction = "return " + luaFunction;
	auto r = static_cast<Lua::StatusCode>(luaL_loadbuffer(l, bufLuaFunction.c_str(), bufLuaFunction.length(), chunkName.c_str())); /* 1 */
	if(r != Lua::StatusCode::Ok) {
		outErrMsg = Lua::CheckString(l, -1);
		Lua::Pop(l); /* 0 */
		return false;
	}
	if(Lua::ProtectedCall(l, 0, 1, outErrMsg) != Lua::StatusCode::Ok) {
		Lua::Pop(l); /* 0 */
		return false;
	}
	return true;
}

Lua::Type Lua::GetType(lua_State *lua, int32_t idx) { return static_cast<Type>(lua_type(lua, idx)); }
const char *Lua::GetTypeName(lua_State *lua, int32_t idx) { return lua_typename(lua, idx); }

void Lua::Pop(lua_State *lua, int32_t n)
{
	if(n <= 0)
		return;
	lua_pop(lua, n);
}

void Lua::Error(lua_State *lua) { lua_error(lua); }
void Lua::Error(lua_State *lua, const std::string &err)
{
	lua_pushstring(lua, err.c_str());
	Error(lua);
}

void Lua::Register(lua_State *lua, const char *name, lua_CFunction f) { lua_register(lua, name, f); }
void Lua::RegisterEnum(lua_State *l, const std::string &name, int32_t val)
{
	lua_pushinteger(l, val);
	lua_setglobal(l, name.c_str());
}

#if 0
std::shared_ptr<luabind::module_> Lua::RegisterLibrary(lua_State *l, const std::string &name,const std::vector<luaL_Reg>& functions)
{
    auto funcSize = functions.size();
#if 1
   auto fCpy = functions;
   fCpy.resize(funcSize);
	if(functions.empty() || functions.back().name != nullptr)
        fCpy.push_back({nullptr,nullptr});
#pragma warning(disable : 4309)
    luaL_newlib(l, fCpy.data());
#pragma warning(default : 4309)
#endif
#if 0
    luaL_Reg fCpy[funcSize+1];
    std::memcpy(fCpy,functions.data(),funcSize);
    if(functions.empty() || functions.back().name != nullptr)
        fCpy[funcSize] = {nullptr,nullptr};
#pragma warning(disable : 4309)
    luaL_newlib(l, fCpy);
#pragma warning(default : 4309)
#endif
	lua_setglobal(l, name.c_str());
	return std::make_shared<luabind::module_>(luabind::module(l, name.c_str()));
}
#endif
std::shared_ptr<luabind::module_> Lua::RegisterLibrary(lua_State *l, const std::string &name, const std::vector<luaL_Reg> &functions)
{
	auto fCpy = functions;
	if(functions.empty() || functions.back().name != nullptr)
		fCpy.push_back({nullptr, nullptr});
#pragma warning(disable : 4309)
	lua_newtable(l);
	luaL_setfuncs(l, fCpy.data(), 0);
#pragma warning(default : 4309)
	lua_setglobal(l, name.c_str());
	return std::make_shared<luabind::module_>(luabind::module(l, name.c_str()));
}
int32_t Lua::CreateReference(lua_State *lua, int32_t t) { return luaL_ref(lua, t); }
void Lua::ReleaseReference(lua_State *lua, int32_t ref, int32_t t) { luaL_unref(lua, t, ref); }
void Lua::Insert(lua_State *lua, int32_t idx) { lua_insert(lua, idx); }

void Lua::GetGlobal(lua_State *lua, const std::string &name) { lua_getglobal(lua, name.c_str()); }
void Lua::SetGlobal(lua_State *lua, const std::string &name) { lua_setglobal(lua, name.c_str()); }
int32_t Lua::GetStackTop(lua_State *lua) { return lua_gettop(lua); }
void Lua::SetStackTop(lua_State *lua, int32_t idx) { lua_settop(lua, idx); }

int32_t Lua::PushTable(lua_State *lua, int32_t n, int32_t idx)
{
	lua_rawgeti(lua, idx, n);
	return GetStackTop(lua);
}
void Lua::SetTableValue(lua_State *lua, int32_t idx) { lua_settable(lua, idx); }
void Lua::GetTableValue(lua_State *lua, int32_t idx) { lua_gettable(lua, idx); }
void Lua::SetTableValue(lua_State *lua, int32_t idx, int32_t n) { lua_rawseti(lua, idx, n); }
std::size_t Lua::GetObjectLength(lua_State *l, const luabind::object &o)
{
	o.push(l);
	auto len = GetObjectLength(l, -1);
	Pop(l);
	return len;
}
std::size_t Lua::GetObjectLength(lua_State *l, int32_t idx) { return lua_rawlen(l, idx); }

static Lua::StatusCode get_protected_table_value(lua_State *lua, int32_t idx, int32_t (*f)(lua_State *))
{
	auto arg = Lua::GetStackTop(lua);
	lua_pushcfunction(lua, f);
	Lua::PushValue(lua, idx); // Table
	Lua::PushValue(lua, arg); // Table[k]
	return static_cast<Lua::StatusCode>(lua_pcall(lua, 2, 1, 0));
}

Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua, int32_t idx)
{
	return get_protected_table_value(lua, idx, [](lua_State *l) -> int32_t {
		Lua::GetTableValue(l, 1);
		return 1;
	});
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key)
{
	Lua::PushString(lua, key); /* 1 */
	auto r = get_protected_table_value(lua, idx, [](lua_State *l) -> int32_t {
		Lua::GetTableValue(l, 1);
		return 1;
	}); /* 2 */
	if(r != StatusCode::Ok) {
		Pop(lua, 2); /* 0 (Error Handler) */
		return r;
	}
	RemoveValue(lua, -2); /* 1 */
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, std::string &val)
{
	val.clear();
	Lua::PushString(lua, key); /* 1 */
	auto r = get_protected_table_value(lua, idx, [](lua_State *l) -> int32_t {
		Lua::GetTableValue(l, 1);
		Lua::CheckString(l, -1);
		return 1;
	}); /* 2 */
	if(r != StatusCode::Ok) {
		Pop(lua, 2); /* 0 (Error Handler) */
		return r;
	}
	val = Lua::ToString(lua, -1);
	Pop(lua, 2); /* 0 */
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, float &val)
{
	val = 0.f;
	Lua::PushString(lua, key);
	auto r = get_protected_table_value(lua, idx, [](lua_State *l) -> int32_t {
		Lua::GetTableValue(l, 1);
		Lua::CheckNumber(l, -1);
		return 1;
	});
	if(r != StatusCode::Ok) {
		Pop(lua, 2); /* 0 (Error Handler) */
		return r;
	}
	val = static_cast<float>(Lua::ToNumber(lua, -1));
	Lua::Pop(lua, 2);
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, int32_t &val)
{
	val = 0;
	Lua::PushString(lua, key);
	auto r = get_protected_table_value(lua, idx, [](lua_State *l) -> int32_t {
		Lua::GetTableValue(l, 1);
		Lua::CheckInt(l, -1);
		return 1;
	});
	if(r != StatusCode::Ok) {
		Pop(lua, 2); /* 0 (Error Handler) */
		return r;
	}
	val = static_cast<int32_t>(Lua::ToInt(lua, -1));
	Lua::Pop(lua, 2);
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua, int32_t idx, const std::string &key, bool &val)
{
	val = 0;
	Lua::PushString(lua, key);
	auto r = get_protected_table_value(lua, idx, [](lua_State *l) -> int32_t {
		Lua::GetTableValue(l, 1);
		Lua::CheckBool(l, -1);
		return 1;
	});
	if(r != StatusCode::Ok) {
		Pop(lua, 2); /* 0 (Error Handler) */
		return r;
	}
	val = Lua::ToBool(lua, -1);
	Lua::Pop(lua, 2);
	return r;
}

void Lua::PushValue(lua_State *lua, int32_t idx) { lua_pushvalue(lua, idx); }
void Lua::RemoveValue(lua_State *lua, int32_t idx) { lua_remove(lua, idx); }
void Lua::InsertValue(lua_State *lua, int32_t idx) { lua_insert(lua, idx); }

int32_t Lua::SetMetaTable(lua_State *lua, int32_t idx) { return lua_setmetatable(lua, idx); }
int32_t Lua::GetNextPair(lua_State *lua, int32_t idx) { return lua_next(lua, idx); }

void Lua::CollectGarbage(lua_State *lua) { lua_gc(lua, LUA_GCCOLLECT, 0); }

#pragma warning(disable : 4309)
void Lua::CreateLibrary(lua_State *l, const luaL_Reg *list) { luaL_newlib(l, list); }
#pragma warning(default : 4309)

void Lua::PushRegistryValue(lua_State *l, int32_t n) { lua_rawgeti(l, RegistryIndex, n); }

static std::string GetPathFromFileName(std::string str)
{
	auto br = str.find_last_of("/\\");
	if(br == std::string::npos || str.length() == 1)
		return "";
	return str.substr(0, br + 1);
}

static std::vector<std::string> s_includeStack;
Lua::StatusCode Lua::ExecuteFile(lua_State *lua, std::string &fInOut, std::string &outErr, int32_t (*traceback)(lua_State *), int32_t numRet, void (*loadErrorHandler)(lua_State *, StatusCode))
{
	fInOut = FileManager::GetNormalizedPath(fInOut);
	auto path = GetPathFromFileName(fInOut);
	if(!path.empty() && (path.front() == '/' || path.front() == '\\'))
		path = path.substr(1);
	s_includeStack.push_back(path);
	auto s = ProtectedCall(lua, [&fInOut](lua_State *l) { return Lua::LoadFile(l, fInOut); }, numRet, outErr, traceback, loadErrorHandler);
	s_includeStack.pop_back();
	return s;
}

Lua::StatusCode Lua::RunString(lua_State *lua, const std::string &str, int32_t retCount, const std::string &chunkName, std::string &outErr, int32_t (*traceback)(lua_State *), void (*loadErrorHandler)(lua_State *, StatusCode))
{
	return ProtectedCall(lua, [&str, &chunkName](lua_State *l) { return static_cast<StatusCode>(luaL_loadbuffer(l, str.c_str(), str.length(), chunkName.c_str())); }, retCount, outErr, traceback, loadErrorHandler);
}

Lua::StatusCode Lua::RunString(lua_State *lua, const std::string &str, const std::string &chunkName, std::string &outErr, int32_t (*traceback)(lua_State *), void (*loadErrorHandler)(lua_State *, StatusCode)) { return RunString(lua, str, 0, chunkName, outErr, traceback, loadErrorHandler); }

void Lua::ExecuteFiles(lua_State *lua, const std::string &subPath, std::string &outErr, int32_t (*traceback)(lua_State *), const std::function<void(StatusCode, const std::string &)> &fCallback)
{
	std::string path = SCRIPT_DIRECTORY_SLASH;
	path += subPath;

	std::vector<std::string> files;
	if(s_precompiledFilesEnabled)
		FileManager::FindFiles((path + "*." + FILE_EXTENSION_PRECOMPILED).c_str(), &files, nullptr);

	if(s_precompiledFilesEnabled) {
		// Add un-compiled lua-files, but only if no compiled version exists
		std::vector<std::string> rFiles;
		FileManager::FindFiles((path + "*." + FILE_EXTENSION).c_str(), &rFiles, nullptr);
		files.reserve(files.size() + rFiles.size());
		for(auto &fName : rFiles) {
			auto lname = fName.substr(0, fName.length() - 3) + FILE_EXTENSION_PRECOMPILED;
			auto it = std::find(files.begin(), files.end(), lname);
			if(it == files.end())
				files.push_back(fName);
		}
	}
	else
		FileManager::FindFiles((path + "*." + FILE_EXTENSION).c_str(), &files, nullptr);

	for(auto &f : files) {
		auto path = subPath + f;
		if(fCallback != nullptr)
			fCallback(ExecuteFile(lua, path, outErr, traceback), path);
	}
}

static void normalize_source(std::string &source)
{
	if(source.empty() == false && source[0] == '@')
		source = source.substr(1);
}

std::string Lua::get_source(const lua_Debug &dbgInfo)
{
	std::string source = dbgInfo.source;
	normalize_source(source);
	return source;
}
std::string Lua::get_short_source(const lua_Debug &dbgInfo)
{
	std::string source = dbgInfo.short_src;
	normalize_source(source);
	return source;
}

std::string Lua::get_current_file(lua_State *l)
{
	lua_Debug info;
	int32_t level = 0;
	while(lua_getstack(l, level, &info)) {
		lua_getinfo(l, "nSl", &info);
		if(ustring::compare(info.what, "C") == false && info.source != nullptr)
			return get_source(info);
		++level;
	}
	return "";
}

std::string Lua::GetIncludePath(const std::string &f)
{
	std::string lf = f;
	if(!s_includeStack.empty() && (lf.empty() || (lf.front() != '/' && lf.front() != '\\')))
		lf = s_includeStack.back() + lf;
	return lf;
}

std::string Lua::GetIncludePath() { return GetIncludePath(""); }

Lua::StatusCode Lua::IncludeFile(lua_State *lua, std::string &fInOut, std::string &outErr, int32_t (*traceback)(lua_State *), int32_t numRet, void (*loadErrorHandler)(lua_State *, StatusCode))
{
	fInOut = GetIncludePath(fInOut);
	return ExecuteFile(lua, fInOut, outErr, traceback, numRet);
}

const char *Lua::GetTypeString(lua_State *l, int32_t n)
{
	auto *arg = lua_tostring(l, n);
	if(arg == nullptr) {
		auto type = static_cast<Type>(lua_type(l, n));
		switch(type) {
		case Type::Nil:
			{
				arg = "nil";
				break;
			}
		case Type::Bool:
			{
				arg = "Boolean";
				break;
			}

		case Type::LightUserData:
			{
				arg = "LightUserData";
				break;
			}
		case Type::Number:
			{
				arg = "Number";
				break;
			}
		case Type::String:
			{
				arg = "String";
				break;
			}
		case Type::Table:
			{
				arg = "Table";
				break;
			}
		case Type::Function:
			{
				arg = "Function";
				break;
			}
		case Type::UserData:
			{
				arg = "UserData";
				break;
			}
		case Type::Thread:
			{
				arg = "Thread";
				break;
			}
		default:
			arg = "Unknown";
		}
	}
	return arg;
}

luabind::weak_ref Lua::CreateWeakReference(const luabind::object &o)
{
	auto *l = o.interpreter();
	o.push(l);
	auto r = luabind::weak_ref(l, l, GetStackTop(l));
	Pop(l, 1);
	return r;
}

void Lua::PushWeakReference(const luabind::weak_ref &ref) { ref.get(ref.state()); }

luabind::object Lua::WeakReferenceToObject(const luabind::weak_ref &ref)
{
	auto *l = ref.state();
	PushWeakReference(ref);
	auto r = luabind::object(luabind::from_stack(l, GetStackTop(l)));
	Pop(l, 1);
	return r;
}

void Lua::RegisterLibraryEnums(lua_State *l, const std::string &libName, const std::unordered_map<std::string, lua_Integer> &enums) { RegisterLibraryValues(l, libName, enums); }

void Lua::GetField(lua_State *l, int32_t idx, const std::string &fieldName) { lua_getfield(l, idx, fieldName.c_str()); }
