#pragma once
#include <Windows.h>

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#ifdef __cplusplus
}
#endif

#define LUAFUNC extern "C" static
#define LUA_PACKAGE_INIT extern "C" _declspec(dllexport)