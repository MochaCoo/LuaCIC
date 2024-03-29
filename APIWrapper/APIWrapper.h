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

#define EXCEPTION_HANDLER
//a = require("APIWrapper")
//b = a.NativeFunction(a.GetApiAddr("user32.dll", "MessageBoxA"), 'stdcall', 'U32', 'U32', 'P', 'P', 'U32')
//b = a.NativeFunction(a.GetApiAddr("user32.dll", "MessageBoxA"), 'stdcall', 'U32', 'U8', 'P', 'P', 'U8')
//b = a.NativeFunction(a.GetApiAddr("user32.dll", "MessageBoxA"), 'win64', 'U32', 'U32', 'P', 'P', 'U32')
//print(b(0, 'aa', 'cc', 4))
//loadsym "apiwrapper", "C:\Users\Mocha\Desktop\Lua5.4.4VS±‡“Î\Release\LuaCPackage.pdb"