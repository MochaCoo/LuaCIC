#include "LuaTestCPackage.h"

LUAFUNC int test(lua_State* L) {

    return 1;
}

static void printtype(lua_State* L,int idx) {
    switch (lua_type(L, idx)) {
    case LUA_TNONE:
        printf("none");
        break;
    case LUA_TNIL:
        printf("nil");
        break;
    case LUA_TBOOLEAN:
        printf("bool:%d", lua_toboolean(L, idx));
        break;
    case LUA_TLIGHTUSERDATA:
        printf("light_userdata:%p", lua_touserdata(L, idx));
        break;
    case LUA_TNUMBER:
        if (lua_isinteger(L, idx))
            printf("int:%lld", lua_tointeger(L, idx));
        else
            printf("num:%lf", lua_tonumber(L, idx));
        break;
    case LUA_TSTRING: {
        size_t len;
        const char* p = lua_tolstring(L, idx, &len);
        if (len == strlen(p)) {
            printf("str:%s", p);
        }
        else {
            printf("hex:");
            for (int j = 0; j < len; j++) {
                if (j == len - 1)
                    printf("%x", p[j]);
                else
                    printf("%x ", p[j]);
            }
        }
        break;
    }
    case LUA_TTABLE: {
        luaL_checkstack(L, 2, "table iteration");
        printf("{");
        lua_pushnil(L);  /* first key */
        int ret = lua_next(L, idx);
        while (ret != 0) {
            /* uses 'key' (at index -2) and 'value' (at index -1) */
            printtype(L, lua_absindex(L, -2));
            printf("--");
            printtype(L, lua_absindex(L, -1));
            lua_pop(L, 1);/* removes 'value'; keeps 'key' for next iteration */
            if (lua_next(L, idx) == 0)
                break;
            printf(", ");
        }
        printf("}");
        break;
    }
    case LUA_TFUNCTION:
        if (lua_iscfunction(L, idx)) {
            printf("cfunc:%p", lua_tocfunction(L, idx));
        }
        else {
            printf("luafunc");
        }
        break;
    case LUA_TUSERDATA:
        printf("full_userdata:%p", lua_touserdata(L, idx));
        break;
    case LUA_TTHREAD:
        printf("thread");
        break;
    }
}
LUAFUNC int printall(lua_State* L) {
    int num = lua_gettop(L);
    for (int i = 1; i <= num; i++) {
        printtype(L, i);
        if(i!=num)
            printf(", ");
    }
    printf("\n");
    return 0;
}

static luaL_Reg registerInfo[] = {
    {"test", test},
    {"printall", printall},
    {NULL,NULL}
};

//luaopen_包名且dll名也为包名 lua:require "包名" package.loadlib("path","luaopen_包名")
//The name of this C function is the string "luaopen_" concatenated with a copy of the module name where each dot is replaced by an underscore.  Moreover, if the module name has a hyphen, its suffix after (and including) the first hyphen is removed.  For instance, if the module name is a.b.c-v2.1, the function name will be luaopen_a_b_c.
LUA_PACKAGE_INIT int luaopen_test(lua_State* L) {
    printf("module:%s path:%s\n", luaL_checkstring(L, 1), luaL_checkstring(L, 2));

    luaL_newlib(L, registerInfo);
    return 1;// Return package table
}

BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        printf("LuaTestCPackage\n");
        break;
    case DLL_THREAD_ATTACH:
        break;
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}