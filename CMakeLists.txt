include(${CMAKE_SOURCE_DIR}/cmake/pr_common.cmake)

option(CONFIG_USE_LUAJIT "Use LuaJIT." ON)

set(PROJ_NAME luasystem)
pr_add_library(${PROJ_NAME} SHARED)

if(CONFIG_USE_LUAJIT)
	pr_add_compile_definitions(${PROJ_NAME} -DUSE_LUAJIT PUBLIC)
	set(LUAJIT_DISABLE_JIT OFF CACHE BOOL "Disable the JIT compiler" FORCE)
else()
	set(LUAJIT_DISABLE_JIT ON CACHE BOOL "Disable the JIT compiler" FORCE)
endif()

pr_add_dependency(${PROJ_NAME} vfilesystem TARGET PUBLIC)
pr_add_third_party_dependency(${PROJ_NAME} luajit PUBLIC)
pr_add_dependency(${PROJ_NAME} luabind TARGET PUBLIC)

pr_init_module(${PROJ_NAME})
pr_add_headers(${PROJ_NAME} "include/")

pr_add_compile_definitions(${PROJ_NAME} -DLUABIND_DYNAMIC_LINK PUBLIC)
pr_add_compile_definitions(${PROJ_NAME} -DDLLLUA_EX)

pr_finalize(${PROJ_NAME})
