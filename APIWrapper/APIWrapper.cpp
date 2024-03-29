#include "APIWrapper.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <intrin.h>
//您要保证lua栈不溢出，用函数lua_checkstack来确保堆栈有足够的空间来压入新元素
//无论何时Lua调用C函数，lua都确保堆栈至少有LUA_MINSTACK额外插槽的空间。LUA_MINSTACK被定义为20，因此通常您不必担心堆栈空间，除非您的代码有循环将元素推入堆栈
//当您调用一个没有固定数量结果的Lua函数时(请参阅lua_call)，Lua确保堆栈有足够的空间容纳所有结果，但它不确保还有额外的空间能继续压入元素。lua_call之后应使用lua_checkstack，再压入元素

/* lib,funcname */
LUAFUNC int GetApiAddr(lua_State* L) {
    const char* libname = luaL_checkstring(L, 1);
    const char* funcname = luaL_checkstring(L, 2);
    HMODULE m = GetModuleHandleA(libname);
    if (m == NULL) {
        m = LoadLibraryA(libname);
        if (m == NULL) {
            lua_pushliteral(L, "LoadLibrary failed");
            lua_error(L);
        }
    }
    auto func = GetProcAddress(m, funcname);
    if (func == NULL) {
        return 0;
    }
    else {
        lua_pushlightuserdata(L, func);
        return 1;
    }
}
enum class ABI :unsigned char { stdcall, mscdecl, win64 };
enum class VarType :unsigned char { nil, void_/*V 仅在返回值中使用*/, pointer/*P*/, constpointer/*CP*/, S8, U8, S16, U16, S32, U32, S64, U64, float32/*F32*/, double64/*F64*/ };
struct NativeFunctionParamStruct
{
    void* funcaddr;
    ABI abi;//CallingConvention
    VarType rettype;
    unsigned char paramcount;
    unsigned short paramsize;
    VarType paramtype[1];
};
static VarType TranslateVarType(const char* type,unsigned short* size) {
#ifdef _WIN64
    if (size != NULL)
        *size += sizeof(uint64_t);
#endif
    if (!strcmp(type, "V")) {
#ifdef _WIN64
        if (size != NULL)
            * size -= sizeof(int64_t);
#endif
        return VarType::void_;
    }
    else if (!strcmp(type, "P")) {
#ifndef _WIN64
        if(size!=NULL)
            *size += sizeof(void*);
#endif
        return VarType::pointer;
    }
    else if (!strcmp(type, "CP")) {
#ifndef _WIN64
        if (size != NULL)
            *size += sizeof(void*);
#endif
        return VarType::constpointer;
    }
    else if (!strcmp(type, "S8")) {//按照参数占用空间计算,下同
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(int32_t);
#endif
        return VarType::S8;
    }
    else if (!strcmp(type, "U8")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(uint32_t);
#endif
        return VarType::U8;
    }
    else if (!strcmp(type, "S16")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(int32_t);
#endif  
        return VarType::S16;
    }
    else if (!strcmp(type, "U16")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(uint32_t);
#endif
        return VarType::U16;
    }
    else if (!strcmp(type, "S32")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(int32_t);
#endif
        return VarType::S32;
    }
    else if (!strcmp(type, "U32")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(uint32_t);
#endif
        return VarType::U32;
    }
    else if (!strcmp(type, "S64")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(int64_t);
#endif
        return VarType::S64;
    }
    else if (!strcmp(type, "U64")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(uint64_t);
#endif
        return VarType::U64;
    }
    else if (!strcmp(type, "F32")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(float);
#endif
        return VarType::float32;
    }
    else if (!strcmp(type, "F64")) {
#ifndef _WIN64
        if (size != NULL)
            * size += sizeof(double);
#endif
        return VarType::double64;
    }
    return VarType::nil;
}

__declspec (noinline) static void __stdcall check(void* arg, size_t size)
{
    printf("stack check\n");
    if ((uintptr_t)arg == (uintptr_t)_AddressOfReturnAddress() + sizeof(void*)) {
        printf("stack error %p %p %p\n", size, arg, _AddressOfReturnAddress());
        system("pause");
    }
}
__declspec (noinline) static uintptr_t __stdcall getcsp()
{
    return (uintptr_t)_AddressOfReturnAddress() + sizeof(void*);
}
LUAFUNC int CallNativeFunction(lua_State* L) {
    auto nf = (NativeFunctionParamStruct*)lua_touserdata(L, lua_upvalueindex(1));//userdata
    if (lua_gettop(L) != nf->paramcount) {
        lua_pushliteral(L, "incorrect argument");
        lua_error(L);
    }
    uintptr_t arg;
#ifdef EXCEPTION_HANDLER
    uintptr_t argcopy;
#endif
#ifdef EXCEPTION_HANDLER
    __try {
#endif
#ifdef _WIN64
        if (nf->paramsize < 0x20) {
            arg = (uintptr_t)_alloca(0x20);//申请的内存在函数结束时会自动释放
            //memset((void*)arg, 0, 0x20);
        }
        else {
            arg = (uintptr_t)_alloca(nf->paramsize);
        }
#else
        //x86release才能正常执行,因为没有栈平衡检测
        _alloca(nf->paramsize);//返回值=esp+0x20
        arg = getcsp();
#endif

#ifdef EXCEPTION_HANDLER
        argcopy = arg;
    }__except (GetExceptionCode() == STATUS_STACK_OVERFLOW) {
        // If the stack overflows, use this function to restore.
        if (_resetstkoflw() == 0) //  _resetstkoflw() returns 0 on failure
        {
            printf("could not reset the stack!\n");
            system("pause");
        }
        lua_pushliteral(L, "_alloca failed");
        lua_error(L);
    }
#endif
    uint64_t win64arg[4] = { 0 };
    float ft;
    /*给NativeFunction的参数赋值*/
    for (decltype(nf->paramcount) i = 0; i < nf->paramcount; i++) {
        switch (nf->paramtype[i]) {
        case VarType::pointer:
            if (lua_isstring(L, i + 1)) {
                *((void**)arg) = (void*)lua_tostring(L, i + 1);
            }
            else if (lua_isinteger(L, i + 1)) {
                *((void**)arg) = (void*)lua_tointeger(L, i + 1);
            }
#ifndef _WIN64
            arg += sizeof(void*);
#endif
            break;
        case VarType::constpointer:
            if (lua_isstring(L, i + 1)) {
                *((void**)arg) = (void*)lua_tostring(L, i + 1);
            }
            else if (lua_isinteger(L, i + 1)) {
                *((void**)arg) = (void*)lua_tointeger(L, i + 1);
            }
#ifndef _WIN64
            arg += sizeof(void*);
#endif
            break;
        case VarType::U8:
        case VarType::U16:
        case VarType::U32:
            *((uintptr_t*)arg) = (uint32_t)luaL_checkinteger(L, i + 1);
#ifndef _WIN64
            arg += sizeof(uint32_t);
#endif
            break;
        case VarType::S8:
            *((uintptr_t*)arg) = (uint8_t)/*防止符号扩展, 下同*/luaL_checkinteger(L, i + 1);
            goto SignParam;
        case VarType::S16:
            *((uintptr_t*)arg) = (uint16_t)luaL_checkinteger(L, i + 1);
            goto SignParam;
        case VarType::S32:
            *((uintptr_t*)arg) = (uint32_t)luaL_checkinteger(L, i + 1);
        SignParam:
#ifndef _WIN64
            arg += sizeof(int32_t);
#endif
            break;
        case VarType::S64:
            *((int64_t*)arg) = (int64_t)luaL_checkinteger(L, i + 1);
            arg += sizeof(int64_t);
            break;
        case VarType::U64:
            *((uint64_t*)arg) = (uint64_t)luaL_checkinteger(L, i + 1);
#ifndef _WIN64
            arg += sizeof(uint64_t);
#endif
            break;
        case VarType::float32:
            ft = (float)luaL_checknumber(L, i + 1);
            *((uintptr_t*)arg) = 0;//高32位清零
            *((uintptr_t*)arg) = *(uintptr_t*)(&ft);
#ifndef _WIN64
            arg += sizeof(float);
#endif
            break;
        case VarType::double64:
            *((double*)arg) = (double)luaL_checknumber(L, i + 1);
#ifndef _WIN64
            arg += sizeof(double);
#endif
            break;
        case VarType::void_:
#ifdef _WIN64
            arg -= sizeof(int64_t);
#endif
            break;
        }
#ifdef _WIN64
        if (i < 4)
            win64arg[i] = *((uint64_t*)arg);
        arg += sizeof(uint64_t);
#endif
    }
#ifdef EXCEPTION_HANDLER
    check((void*)arg, nf->paramsize);
#endif
    float fret = 0;double dret = 0;uint64_t ret = 0;
    PVOID ea = 0;
#ifdef EXCEPTION_HANDLER
    __try {
#endif
        /*call NativeFunction*/
#ifdef _WIN64//考虑支持win64前4参数为浮点数的情况
        if (nf->abi == ABI::win64) {
            if (nf->rettype == VarType::float32)
                fret = ((float(*)(uint64_t, uint64_t, uint64_t, uint64_t))(nf->funcaddr))(win64arg[0], win64arg[1], win64arg[2], win64arg[3]);
            else if (nf->rettype == VarType::double64)
                dret = ((double(*)(uint64_t, uint64_t, uint64_t, uint64_t))(nf->funcaddr))(win64arg[0], win64arg[1], win64arg[2], win64arg[3]);
            else
                ret = ((uint64_t(*)(uint64_t, uint64_t, uint64_t, uint64_t))(nf->funcaddr))(win64arg[0], win64arg[1], win64arg[2], win64arg[3]);
        }
        
#else
        if (nf->abi == ABI::stdcall || nf->abi == ABI::mscdecl) {
            if(nf->rettype==VarType::float32)
                fret = ((float(*)())(nf->funcaddr))();
            else if(nf->rettype == VarType::double64)
                dret = ((double(*)())(nf->funcaddr))();
            else
                ret = ((uint64_t(*)())(nf->funcaddr))();
        }
#endif

#ifdef EXCEPTION_HANDLER
    }
    __except (ea = GetExceptionInformation()->ExceptionRecord->ExceptionAddress, EXCEPTION_EXECUTE_HANDLER) {
        luaL_error(L, "ErrorCode:%p Addr:%p NativeFunction:%p callstack:%p", GetExceptionCode(), ea, nf->funcaddr, argcopy);
    }
#endif
    /*push return value on stack*/
    switch (nf->rettype) {
    case VarType::pointer:
    case VarType::constpointer:
        lua_pushinteger(L, (lua_Integer)(void*)ret);
        break;
    case VarType::U8:
        lua_pushinteger(L, (lua_Integer)(uint8_t)ret);
        break;
    case VarType::U16:
        lua_pushinteger(L, (lua_Integer)(uint16_t)ret);
        break;
    case VarType::U32:
        lua_pushinteger(L, (lua_Integer)(uint32_t)ret);
        break;
    case VarType::U64:
        lua_pushinteger(L, (lua_Integer)(uint64_t)ret);
        break;
    case VarType::S8:
        lua_pushinteger(L, (lua_Integer)(int8_t)ret);
        break;
    case VarType::S16:
        lua_pushinteger(L, (lua_Integer)(int16_t)ret);
        break;
    case VarType::S32:
        lua_pushinteger(L, (lua_Integer)(int32_t)ret);
        break;
    case VarType::S64:
        lua_pushinteger(L, (lua_Integer)(int64_t)ret);
        break;
    case VarType::float32:
        lua_pushnumber(L, fret);
        break;
    case VarType::double64:
        lua_pushnumber(L, dret);
        break;
    case VarType::void_:
        return 0;
    }
    return 1;
}
/* funcaddr,abi,rettype,paramtype */
LUAFUNC int NativeFunction(lua_State* L) {
    int n = lua_gettop(L);    /* number of arguments */
    // pushes onto stack a userdata
    auto nf = (NativeFunctionParamStruct*)lua_newuserdatauv(L, sizeof(NativeFunctionParamStruct) + sizeof(VarType) * (n - 3), 0);
    if (n<3) {
        lua_pushliteral(L, "at least 3 argument");
        lua_error(L);
    }
    nf->paramcount = n - 3;

    if (lua_islightuserdata(L, 1)) {//funcaddr
        nf->funcaddr = lua_touserdata(L, 1);
    }
    else {
        nf->funcaddr = (void*)luaL_checkinteger(L, 1);
    }
    

    const char* abi = luaL_checkstring(L, 2);//abi
#ifdef _WIN64
    nf->abi = ABI::win64;
    /*if (!strcmp(abi, "win64")) {
        nf->abi = ABI::win64;
    }*/
#else
    if (!strcmp(abi, "stdcall")) {
        nf->abi = ABI::stdcall;
    }
    else if (!strcmp(abi, "mscdecl")) {
        nf->abi = ABI::mscdecl;
    }
    else {
        lua_pushliteral(L, "error abi");
        lua_error(L);
    }
#endif
    
    nf->rettype = TranslateVarType(luaL_checkstring(L, 3), NULL);//rettype
    if (nf->rettype == VarType::nil) {
        lua_pushliteral(L, "error rettype");
        lua_error(L);
    }
    nf->paramsize = 0;
    for (decltype(nf->paramcount) i = 0; i < n-3; i++) {
        nf->paramtype[i] = TranslateVarType(luaL_checkstring(L, i + 3 + 1), &nf->paramsize);//paramtype paramsize
        if (nf->paramtype[i] == VarType::nil) {
            lua_pushliteral(L, "error paramtype");
            lua_error(L);
        }
    }

    if (nf->paramsize > 1024) {
        lua_pushliteral(L, "too much argument");
        lua_error(L);
    }

    lua_pushcclosure(L, CallNativeFunction, 1);
    return 1;                   /* number of results */
}
/*
m=require('APIWrapper')
a=m.NativeFunction(m.GetApiAddr('user32.dll','MessageBoxA'),'stdcall','U32','U64','P','P','U64')
//a=m.NativeFunction(m.GetApiAddr('user32.dll','MessageBoxA'),'stdcall','U32','U32','P','P','U32')
a(0,'an','vvv',3)
*/
static luaL_Reg registerInfo[] = {
    {"NativeFunction", NativeFunction},/* funcaddr: lightuserdata|integer, abi: ABI, rettype: VarType, paramtype: VarType */
    {"GetApiAddr", GetApiAddr},/* lib: string, funcname: string */
    {NULL,NULL}
};

//luaopen_包名且dll名也为包名 lua:require "包名" package.loadlib("path","luaopen_包名")
LUA_PACKAGE_INIT int luaopen_APIWrapper(lua_State* L) {
    luaL_newlib(L, registerInfo);
    return 1;
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
#ifdef EXCEPTION_HANDLER
        printf("APIWrapperMain\n");
#endif
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


