// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#pragma once

//#define LUABIND_NO_EXCEPTIONS
#ifndef LUABIND_DYNAMIC_LINK
#define LUABIND_DYNAMIC_LINK
#endif

//#define USE_LUAJIT

#ifdef DLLLUA_EX
#ifdef __linux__
#define DLLLUA __attribute__((visibility("default")))
#else
#define DLLLUA __declspec(dllexport)
#endif
#else
#ifdef __linux__
#define DLLLUA
#else
#define DLLLUA __declspec(dllimport)
#endif
#endif
