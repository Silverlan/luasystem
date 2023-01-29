/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __LUADEFINITIONS_H__
#define __LUADEFINITIONS_H__

//#define LUABIND_NO_EXCEPTIONS
#define LUABIND_DYNAMIC_LINK

#define USE_LUAJIT

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
#endif
