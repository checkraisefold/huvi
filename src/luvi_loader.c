#include "luvi.h"

LUALIB_API int lua_loadstring(lua_State *L)
{
    size_t l = 0;
    size_t bytecodeLen = 0;
    const char* s = luaL_checklstring(L, 1, &l);
    const char* chunkname = luaL_optstring(L, 2, s);

    char* bytecode = luau_compile(s, l, NULL, &bytecodeLen);
    if (luau_load(L, chunkname, bytecode, bytecodeLen, 0) == 0)
        return 1;

    lua_pushnil(L);
    lua_insert(L, -2); // put before error message
    return 2;          // return nil plus error message
}

void luvi_openlibs(lua_State *L) {
  luaL_openlibs(L);
}
