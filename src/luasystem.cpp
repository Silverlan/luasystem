/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "luasystem_file.h"
#include <fsys/filesystem.h>
#include "impl_luajit_definitions.hpp"
#include <sharedutils/util_string.h>

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

void Lua::CloseState(lua_State *lua)
{
	lua_close(lua);
}

static bool s_precompiledFilesEnabled = true;
void Lua::set_precompiled_files_enabled(bool bEnabled) {s_precompiledFilesEnabled = bEnabled;}
bool Lua::are_precompiled_files_enabled() {return s_precompiledFilesEnabled;}

void Lua::get_global_nested_library(lua_State *l,const std::string &name)
{
	std::vector<std::string> libs;
	ustring::explode(name,".",libs);
	if(libs.empty() == false)
	{
		Lua::GetGlobal(l,libs.front()); /* 1 */
		if(Lua::IsSet(l,-1) == false)
		{
			Lua::Pop(l,1); /* 0 */
			Lua::PushNil(l);
			return;
		}
		if(libs.size() == 1)
			return;
		for(auto it=libs.begin() +1;it!=libs.end();++it)
		{
			auto bLast = (it == libs.end() -1);
			auto &lib = *it;
			auto t = Lua::GetStackTop(l);
			auto status = Lua::GetProtectedTableValue(l,t,lib);
			if(status != Lua::StatusCode::Ok)
			{
				Lua::PushNil(l);
				return;
			}
			Lua::RemoveValue(l,-2);
			if(bLast)
				return;
		}
	}
	Lua::PushNil(l);
}

Lua::StatusCode Lua::LoadFile(lua_State *lua,std::string &fInOut,fsys::SearchFlags includeFlags,fsys::SearchFlags excludeFlags)
{
	if(s_precompiledFilesEnabled)
	{
		if(fInOut.length() > 3 && fInOut.substr(fInOut.length() -4) == ".lua")
		{
			auto cpath = fInOut.substr(0,fInOut.length() -4) +".clua";
			if(FileManager::Exists("lua\\" +cpath,includeFlags,excludeFlags) == true)
				fInOut = cpath;
		}
	}
	fInOut = FileManager::GetNormalizedPath("lua\\" +fInOut);
	auto f = FileManager::OpenFile(fInOut.c_str(),"rb",includeFlags,excludeFlags);
	if(f == nullptr)
	{
		lua_pushfstring(lua,"cannot open %s: File not found!",fInOut.c_str());
		return StatusCode::ErrorFile;
	}
	auto nf = fInOut;
	auto fReal = std::dynamic_pointer_cast<VFilePtrInternalReal>(f);
	if(fReal != nullptr)
	{
		// We need the full path (at least relative to the program), otherwise the ZeroBrane debugger may not work in some cases
		nf = FileManager::GetCanonicalizedPath(fReal->GetPath());
		static auto len = FileManager::GetProgramPath().length();
		nf = nf.substr(len +1);
	}
	auto l = f->GetSize();
	std::vector<char> buf(l);
	f->Read(buf.data(),l);
	return static_cast<StatusCode>(luaL_loadbuffer(lua,buf.data(),l,nf.c_str()));
}

void Lua::Call(lua_State *lua,int32_t nargs,int32_t nresults) {lua_call(lua,nargs,nresults);}

Lua::StatusCode Lua::ProtectedCall(lua_State *lua,const std::function<StatusCode(lua_State*)> &pushArgs,int32_t numResults,int32_t(*traceback)(lua_State*))
{
	int32_t tracebackIdx = 0;
	if(traceback != nullptr)
	{
		lua_pushcfunction(lua,traceback);
		tracebackIdx = GetStackTop(lua);
	}
	auto top = GetStackTop(lua);
	int32_t s = 0;
	if(pushArgs != nullptr)
	{
		s = umath::to_integral(pushArgs(lua));
		if(s != 0)
		{
			if(traceback != nullptr)
				RemoveValue(lua,tracebackIdx);
			auto newTop = GetStackTop(lua);
			if(newTop > top)
				Pop(lua,newTop -top);
			return static_cast<StatusCode>(s);
		}
	}

	auto numArgs = GetStackTop(lua) -top -1;
	auto r = lua_pcall(lua,numArgs,numResults,tracebackIdx);
	if(traceback != nullptr)
		RemoveValue(lua,tracebackIdx);
	if(static_cast<StatusCode>(r) != StatusCode::Ok)
		Lua::Pop(lua,1); // Pop error message from stack
	return static_cast<StatusCode>(r);
}
Lua::StatusCode Lua::ProtectedCall(lua_State *lua,int32_t nargs,int32_t nresults,std::string *err)
{
	auto s = lua_pcall(lua,nargs,nresults,0);
	if(s != 0)
	{
		if(err == nullptr)
			return static_cast<StatusCode>(s);
		*err = lua_tostring(lua,-1);
		return static_cast<StatusCode>(s);
	}
	return static_cast<StatusCode>(s);
}

int32_t Lua::CreateTable(lua_State *lua) {lua_newtable(lua); return Lua::GetStackTop(lua);}
int32_t Lua::CreateMetaTable(lua_State *lua,const std::string &tname) {return luaL_newmetatable(lua,tname.c_str());}

bool Lua::IsBool(lua_State *lua,int32_t idx) {return lua_isboolean(lua,idx);}
bool Lua::IsCFunction(lua_State *lua,int32_t idx) {return (lua_iscfunction(lua,idx) == 1) ? true : false;}
bool Lua::IsFunction(lua_State *lua,int32_t idx) {return lua_isfunction(lua,idx);}
bool Lua::IsNil(lua_State *lua,int32_t idx) {return lua_isnil(lua,idx);}
bool Lua::IsNone(lua_State *lua,int32_t idx) {return lua_isnone(lua,idx);}
bool Lua::IsSet(lua_State *lua,int32_t idx) {return !lua_isnoneornil(lua,idx);}
bool Lua::IsNumber(lua_State *lua,int32_t idx) {return (lua_isnumber(lua,idx) == 1) ? true : false;}
bool Lua::IsString(lua_State *lua,int32_t idx) {return (lua_isstring(lua,idx) == 1) ? true : false;}
bool Lua::IsTable(lua_State *lua,int32_t idx) {return lua_istable(lua,idx);}
bool Lua::IsUserData(lua_State *lua,int32_t idx) {return (lua_isuserdata(lua,idx) == 1) ? true : false;}

void Lua::PushBool(lua_State *lua,bool b) {lua_pushboolean(lua,b);}
void Lua::PushCFunction(lua_State *lua,lua_CFunction f) {lua_pushcfunction(lua,f);}
void Lua::PushInt(lua_State *lua,ptrdiff_t i) {lua_pushinteger(lua,i);}
void Lua::PushNil(lua_State *lua) {lua_pushnil(lua);}
void Lua::PushNumber(lua_State *lua,float f) {lua_pushnumber(lua,f);}
void Lua::PushNumber(lua_State *lua,double d) {lua_pushnumber(lua,d);}
void Lua::PushString(lua_State *lua,const char *str) {lua_pushstring(lua,str);}
void Lua::PushString(lua_State *lua,const std::string &str) {lua_pushstring(lua,str.c_str());}

bool Lua::ToBool(lua_State *lua,int32_t idx) {return (lua_toboolean(lua,idx) == 1) ? true : false;}
lua_CFunction Lua::ToCFunction(lua_State *lua,int32_t idx) {return lua_tocfunction(lua,idx);}
ptrdiff_t Lua::ToInt(lua_State *lua,int32_t idx) {return lua_tointeger(lua,idx);}
double Lua::ToNumber(lua_State *lua,int32_t idx) {return lua_tonumber(lua,idx);}
const char *Lua::ToString(lua_State *lua,int32_t idx) {return lua_tostring(lua,idx);}
void *Lua::ToUserData(lua_State *lua,int32_t idx) {return lua_touserdata(lua,idx);}

ptrdiff_t Lua::CheckInt(lua_State *lua,int32_t idx)
{
	Lua::CheckNumber(lua,idx);
	return lua_tointeger(lua,idx);
}
double Lua::CheckNumber(lua_State *lua,int32_t idx) {return luaL_checknumber(lua,idx);}
const char *Lua::CheckString(lua_State *lua,int32_t idx) {return luaL_checkstring(lua,idx);}
void Lua::CheckType(lua_State *lua,int32_t narg,int32_t t) {luaL_checktype(lua,narg,t);}
void Lua::CheckTable(lua_State *lua,int32_t narg) {CheckType(lua,narg,LUA_TTABLE);}
void *Lua::CheckUserData(lua_State *lua,int32_t narg,const std::string &tname) {return luaL_checkudata(lua,narg,tname.c_str());}
void Lua::CheckUserData(lua_State *lua,int32_t n) {return luaL_checkuserdata(lua,n);}
void Lua::CheckFunction(lua_State *lua,int32_t idx) {luaL_checktype(lua,(idx),LUA_TFUNCTION);}
void Lua::CheckNil(lua_State *lua,int32_t idx) {luaL_checktype(lua,(idx),LUA_TNIL);}
void Lua::CheckThread(lua_State *lua,int32_t idx) {luaL_checktype(lua,(idx),LUA_TTHREAD);}
bool Lua::CheckBool(lua_State *lua,int32_t idx)
{
	if(!lua_isboolean(lua,idx))
	{
		auto *msg = lua_pushfstring(lua,"%s expected, got %s",lua_typename(lua,LUA_TBOOLEAN),lua_typename(lua,lua_type(lua,idx)));
		luaL_argerror(lua,idx,msg);
	}
	return (lua_toboolean(lua,idx) == 1) ? true : false;
}
bool Lua::PushLuaFunctionFromString(lua_State *l,const std::string &luaFunction,const std::string &chunkName,std::string &outErrMsg)
{
	auto bufLuaFunction = "return " +luaFunction;
	auto r = static_cast<Lua::StatusCode>(luaL_loadbuffer(l,bufLuaFunction.c_str(),bufLuaFunction.length(),chunkName.c_str())); /* 1 */
	if(r != Lua::StatusCode::Ok)
	{
		outErrMsg = Lua::CheckString(l,-1);
		Lua::Pop(l); /* 0 */
		return false;
	}
	if(Lua::ProtectedCall(l,0,1,&outErrMsg) != Lua::StatusCode::Ok)
	{
		Lua::Pop(l); /* 0 */
		return false;
	}
	return true;
}

Lua::Type Lua::GetType(lua_State *lua,int32_t idx) {return static_cast<Type>(lua_type(lua,idx));}
const char *Lua::GetTypeName(lua_State *lua,int32_t idx) {return lua_typename(lua,idx);}

void Lua::Pop(lua_State *lua,int32_t n) {lua_pop(lua,n);}

void Lua::Error(lua_State *lua) {lua_error(lua);}
void Lua::Error(lua_State *lua,const std::string &err)
{
	lua_pushstring(lua,err.c_str());
	Error(lua);
}

void Lua::Register(lua_State *lua,const char *name,lua_CFunction f) {lua_register(lua,name,f);}
void Lua::RegisterEnum(lua_State *l,const std::string &name,int32_t val)
{
	lua_pushinteger(l,val);
	lua_setglobal(l,name.c_str());
}

std::shared_ptr<luabind::module_> Lua::RegisterLibrary(lua_State *l,const std::string &name,const std::vector<luaL_Reg> &functions)
{
	auto fCpy = functions;
	if(functions.empty() || functions.back().name != nullptr)
		fCpy.push_back({nullptr,nullptr});
#pragma warning(disable: 4309)
	luaL_newlib(l,fCpy.data());
#pragma warning(default: 4309)
	lua_setglobal(l,name.c_str());
	return std::make_shared<luabind::module_>(luabind::module(l,name.c_str()));
}
int32_t Lua::CreateReference(lua_State *lua,int32_t t) {return luaL_ref(lua,t);}
void Lua::ReleaseReference(lua_State *lua,int32_t ref,int32_t t) {luaL_unref(lua,t,ref);}
void Lua::Insert(lua_State *lua,int32_t idx) {lua_insert(lua,idx);}

void Lua::GetGlobal(lua_State *lua,const std::string &name) {lua_getglobal(lua,name.c_str());}
void Lua::SetGlobal(lua_State *lua,const std::string &name) {lua_setglobal(lua,name.c_str());}
int32_t Lua::GetStackTop(lua_State *lua) {return lua_gettop(lua);}
void Lua::SetStackTop(lua_State *lua,int32_t idx) {lua_settop(lua,idx);}

int32_t Lua::PushTable(lua_State *lua,int32_t n,int32_t idx)
{
	lua_rawgeti(lua,idx,n);
	return GetStackTop(lua);
}
void Lua::SetTableValue(lua_State *lua,int32_t idx) {lua_settable(lua,idx);}
void Lua::GetTableValue(lua_State *lua,int32_t idx) {lua_gettable(lua,idx);}
void Lua::SetTableValue(lua_State *lua,int32_t idx,int32_t n) {lua_rawseti(lua,idx,n);}
std::size_t Lua::GetObjectLength(lua_State *l,int32_t idx)
{
	return lua_rawlen(l,idx);
}

static Lua::StatusCode get_protected_table_value(lua_State *lua,int32_t idx,int32_t(*f)(lua_State*))
{
	auto arg = Lua::GetStackTop(lua);
	lua_pushcfunction(lua,f);
	Lua::PushValue(lua,idx); // Table
	Lua::PushValue(lua,arg); // Table[k]
	return static_cast<Lua::StatusCode>(lua_pcall(lua,2,1,0));
}

Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua,int32_t idx)
{
	return get_protected_table_value(lua,idx,[](lua_State *l) -> int32_t {
		Lua::GetTableValue(l,1);
		return 1;
	});
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua,int32_t idx,const std::string &key)
{
	Lua::PushString(lua,key); /* 1 */
	auto r = get_protected_table_value(lua,idx,[](lua_State *l) -> int32_t {
		Lua::GetTableValue(l,1);
		return 1;
	}); /* 2 */
	if(r != StatusCode::Ok)
	{
		Pop(lua,2); /* 0 (Error Handler) */
		return r;
	}
	RemoveValue(lua,-2); /* 1 */
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua,int32_t idx,const std::string &key,std::string &val)
{
	val.clear();
	Lua::PushString(lua,key); /* 1 */
	auto r = get_protected_table_value(lua,idx,[](lua_State *l) -> int32_t {
		Lua::GetTableValue(l,1);
		Lua::CheckString(l,-1);
		return 1;
	}); /* 2 */
	if(r != StatusCode::Ok)
	{
		Pop(lua,2); /* 0 (Error Handler) */
		return r;
	}
	val = Lua::ToString(lua,-1);
	Pop(lua,2); /* 0 */
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua,int32_t idx,const std::string &key,float &val)
{
	val = 0.f;
	Lua::PushString(lua,key);
	auto r = get_protected_table_value(lua,idx,[](lua_State *l) -> int32_t {
		Lua::GetTableValue(l,1);
		Lua::CheckNumber(l,-1);
		return 1;
	});
	if(r != StatusCode::Ok)
	{
		Pop(lua,2); /* 0 (Error Handler) */
		return r;
	}
	val = static_cast<float>(Lua::ToNumber(lua,-1));
	Lua::Pop(lua,2);
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua,int32_t idx,const std::string &key,int32_t &val)
{
	val = 0;
	Lua::PushString(lua,key);
	auto r = get_protected_table_value(lua,idx,[](lua_State *l) -> int32_t {
		Lua::GetTableValue(l,1);
		Lua::CheckInt(l,-1);
		return 1;
	});
	if(r != StatusCode::Ok)
	{
		Pop(lua,2); /* 0 (Error Handler) */
		return r;
	}
	val = static_cast<int32_t>(Lua::ToInt(lua,-1));
	Lua::Pop(lua,2);
	return r;
}
Lua::StatusCode Lua::GetProtectedTableValue(lua_State *lua,int32_t idx,const std::string &key,bool &val)
{
	val = 0;
	Lua::PushString(lua,key);
	auto r = get_protected_table_value(lua,idx,[](lua_State *l) -> int32_t {
		Lua::GetTableValue(l,1);
		Lua::CheckBool(l,-1);
		return 1;
	});
	if(r != StatusCode::Ok)
	{
		Pop(lua,2); /* 0 (Error Handler) */
		return r;
	}
	val = Lua::ToBool(lua,-1);
	Lua::Pop(lua,2);
	return r;
}

void Lua::PushValue(lua_State *lua,int32_t idx) {lua_pushvalue(lua,idx);}
void Lua::RemoveValue(lua_State *lua,int32_t idx) {lua_remove(lua,idx);}
void Lua::InsertValue(lua_State *lua,int32_t idx) {lua_insert(lua,idx);}

int32_t Lua::SetMetaTable(lua_State *lua,int32_t idx) {return lua_setmetatable(lua,idx);}
int32_t Lua::GetNextPair(lua_State *lua,int32_t idx) {return lua_next(lua,idx);}

void Lua::CollectGarbage(lua_State *lua) {lua_gc(lua,LUA_GCCOLLECT,0);}

#pragma warning(disable: 4309)
void Lua::CreateLibrary(lua_State *l,const luaL_Reg *list) {luaL_newlib(l,list);}
#pragma warning(default: 4309)

void Lua::PushRegistryValue(lua_State *l,int32_t n) {lua_rawgeti(l,RegistryIndex,n);}

static std::string GetPathFromFileName(std::string str)
{
	auto br = str.find_last_of("/\\");
	if(br == std::string::npos) return "";
	return str.substr(0,br +1);
}

static std::vector<std::string> s_includeStack;
Lua::StatusCode Lua::ExecuteFile(lua_State *lua,std::string &fInOut,int32_t(*traceback)(lua_State*),int32_t numRet)
{
	fInOut = FileManager::GetNormalizedPath(fInOut);
	auto path = GetPathFromFileName(fInOut);
	s_includeStack.push_back(path);
	auto s = ProtectedCall(lua,[&fInOut](lua_State *l) {
		return Lua::LoadFile(l,fInOut);
	},numRet,traceback);
	s_includeStack.pop_back();
	return static_cast<StatusCode>(s);
}

Lua::StatusCode Lua::RunString(lua_State *lua,const std::string &str,int32_t retCount,const std::string &chunkName,int32_t(*traceback)(lua_State*))
{
	auto s = ProtectedCall(lua,[&str,&chunkName](lua_State *l) {
		return static_cast<StatusCode>(luaL_loadbuffer(l,str.c_str(),str.length(),chunkName.c_str()));
	},retCount,traceback);
	return static_cast<StatusCode>(s);
}

Lua::StatusCode Lua::RunString(lua_State *lua,const std::string &str,const std::string &chunkName,int32_t(*traceback)(lua_State*)) {return RunString(lua,str,0,chunkName,traceback);}

void Lua::ExecuteFiles(lua_State *lua,const std::string &subPath,int32_t(*traceback)(lua_State*),const std::function<void(StatusCode,const std::string&)> &fCallback)
{
	std::string path = "lua\\";
	path += subPath;

	std::vector<std::string> files;
	if(s_precompiledFilesEnabled)
		FileManager::FindFiles((path +"*.clua").c_str(),&files,nullptr);

	if(s_precompiledFilesEnabled)
	{
		// Add un-compiled lua-files, but only if no compiled version exists
		std::vector<std::string> rFiles;
		FileManager::FindFiles((path +"*.lua").c_str(),&rFiles,nullptr);
		files.reserve(files.size() +rFiles.size());
		for(auto &fName : rFiles)
		{
			auto lname = fName.substr(0,fName.length() -3) +"clua";
			auto it = std::find(files.begin(),files.end(),lname);
			if(it == files.end())
				files.push_back(fName);
		}
	}
	else
		FileManager::FindFiles((path +"*.lua").c_str(),&files,nullptr);

	for(auto &f : files)
	{
		auto path = subPath +f;
		if(fCallback != nullptr)
			fCallback(ExecuteFile(lua,path,traceback),path);
	}
}

std::string Lua::get_current_file(lua_State *l)
{
	lua_Debug info;
	int32_t level = 0;
	while(lua_getstack(l,level,&info))
	{
		lua_getinfo(l, "nSl", &info);
		if(ustring::compare(info.what,"C") == false && info.source != nullptr)
			return info.source;
		++level;
	}
	return "";
}

std::string Lua::GetIncludePath(const std::string &f)
{
	std::string lf = f;
	if(!s_includeStack.empty() && (lf.empty() || (lf.front() != '/' && lf.front() != '\\')))
		lf = s_includeStack.back() +lf;
	return lf;
}

std::string Lua::GetIncludePath() {return GetIncludePath("");}

Lua::StatusCode Lua::IncludeFile(lua_State *lua,std::string &fInOut,int32_t(*traceback)(lua_State*),int32_t numRet)
{
	fInOut = GetIncludePath(fInOut);
	return ExecuteFile(lua,fInOut,traceback,numRet);
}

const char *Lua::GetTypeString(lua_State *l,int32_t n)
{
	auto *arg = lua_tostring(l,n);
	if(arg == nullptr)
	{
		auto type = static_cast<Type>(lua_type(l,n));
		switch(type)
		{
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
	auto r = luabind::weak_ref(l,l,GetStackTop(l));
	Pop(l,1);
	return r;
}

void Lua::PushWeakReference(const luabind::weak_ref &ref) {ref.get(ref.state());}

luabind::object Lua::WeakReferenceToObject(const luabind::weak_ref &ref)
{
	auto *l = ref.state();
	PushWeakReference(ref);
	auto r = luabind::object(luabind::from_stack(l,GetStackTop(l)));
	Pop(l,1);
	return r;
}

void Lua::RegisterLibraryEnums(lua_State *l,const std::string &libName,const std::unordered_map<std::string,lua_Integer> &enums)
{
	get_global_nested_library(l,libName);
	if(Lua::IsNil(l,-1))
		throw std::runtime_error("No library '" +libName +" found!");
	auto t = GetStackTop(l);
	if(!IsNil(l,t))
	{
		for(auto &pair : enums)
		{
			PushString(l,pair.first);
			PushInt(l,pair.second);
			SetTableValue(l,t);
		}
	}
	Pop(l,1);
}

void Lua::GetField(lua_State *l,int32_t idx,const std::string &fieldName)
{
	lua_getfield(l,idx,fieldName.c_str());
}
