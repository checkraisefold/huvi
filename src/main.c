/*
 *  Copyright 2014 The Luvit Authors. All Rights Reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include "luvi.h"
#include "luv.h"
#include "lenv.c"
#include "luvi.c"

#include "snapshot.c"

int luaopen_miniz(lua_State *L);

#ifdef WITH_CUSTOM
int luvi_custom(lua_State* L);
#endif

static int luvi_traceback(lua_State *L) {
  if (!lua_isstring(L, 1))  /* 'message' not a string? */
    return 1;  /* keep it intact */
  luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 1);
  lua_getfield(L, -1, "debug");
  lua_remove(L, -2);
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return 1;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return 1;
  }
  lua_pushvalue(L, 1);  /* pass error message */
  lua_pushinteger(L, 2);  /* skip this function and traceback */
  lua_call(L, 2, 1);  /* call debug.traceback */
  return 1;
}

static lua_State* vm_acquire(){
  lua_State*L = luaL_newstate();
  if (L == NULL)
    return L;

  // Add in the lua standard and compat libraries
  luvi_openlibs(L);

  // create an env table to push our std lib into
  lua_createtable(L, 0, 7);
  lua_pushvalue(L, -1);
  lua_setglobal(L, "luvi");

  // load luv into uv in advance so that the metatables for async work.
  luaopen_luv(L);
  lua_setglobal(L, "uv");

  luaopen_env(L);
  lua_setfield(L, -2, "env");

  luaopen_miniz(L);
  lua_setglobal(L, "miniz");

  lua_pushcfunction(L, lua_loadstring, NULL);
  lua_setglobal(L, "loadstring");

  //lua_pushcfunction(L, luaopen_snapshot, NULL);
  //lua_setfield(L, -2, "snapshot");

#ifdef WITH_LPEG
  lua_pushcfunction(L, luaopen_lpeg, NULL);
  lua_setfield(L, -2, "lpeg");
#endif

#ifdef WITH_PCRE2
  lua_pushcfunction(L, luaopen_rex_pcre2, NULL);
  lua_pushvalue(L, -1);
  lua_setfield(L, -3, "rex_pcre2");
  lua_setfield(L, -2, "rex");
#endif

#ifdef WITH_OPENSSL
  // Store openssl module definition at preload.openssl
  lua_pushcfunction(L, luaopen_openssl, NULL);
  lua_setfield(L, -2, "openssl");
#endif

#ifdef WITH_ZLIB
  // Store zlib module definition at preload.zlib
  lua_pushcfunction(L, luaopen_zlib, NULL);
  lua_setfield(L, -2, "zlib");
#endif

#ifdef WITH_WINSVC
  // Store luvi module definition at preload.winsvc
  lua_pushcfunction(L, luaopen_winsvc, NULL);
  lua_setfield(L, -2, "winsvc");
  lua_pushcfunction(L, luaopen_winsvcaux, NULL);
  lua_setfield(L, -2, "winsvcaux");
#endif

  // Store luvi module definition at luvi.info
  luaopen_luvi(L);
  lua_setfield(L, -2, "info");

  luaopen_luvipath(L);
  lua_getglobal(L, "luvi");
  lua_insert(L, 1);
  lua_setfield(L, -2, "luvipath");

  luaopen_luvibundle(L);
  lua_getglobal(L, "luvi");
  lua_insert(L, 1);
  lua_setfield(L, -2, "luvibundle");

  luaopen_init(L);
  lua_getglobal(L, "luvi");
  lua_insert(L, 1);
  lua_setfield(L, -2, "init");

#ifdef WITH_LJ_VMDEF
  lua_pushcfunction(L, luaopen_vmdef, NULL);
  lua_setfield(L, -2, "jit.vmdef");
#endif

#ifdef WITH_CUSTOM
  luvi_custom(L);
#endif
  return L;
}

static void vm_release(lua_State*L) {
  lua_close(L);
}

int main(int argc, char* argv[] ) {

  lua_State* L;
  int index;
  int res;
  int errfunc;

  // Hooks in libuv that need to be done in main.
  argv = uv_setup_args(argc, argv);

  luv_set_thread_cb(vm_acquire, vm_release);
  // Create the lua state.
  L = vm_acquire();
  if (L == NULL) {
    fprintf(stderr, "luaL_newstate has failed\n");
    return 1;
  }

  /* push debug function */
  lua_pushcfunction(L, luvi_traceback, NULL);
  errfunc = lua_gettop(L);

  // Load the init.lua script
  const char toLoad[] = "return luvi.init(...)";
  size_t bytecodeLen = 0;
  char* bytecode = luau_compile(toLoad, sizeof(toLoad) - 1, NULL, &bytecodeLen);
  if (luau_load(L, "=luvi_init_vm", bytecode, bytecodeLen, 0)) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    vm_release(L);
    return -1;
  }

  // Pass the command-line arguments to init as a zero-indexed table
  lua_createtable (L, argc, 0);
  for (index = 0; index < argc; index++) {
    lua_pushstring(L, argv[index]);
    lua_rawseti(L, -2, index);
  }

  // Start the main script.
  if (lua_pcall(L, 1, 1, errfunc)) {
    fprintf(stderr, "%s\n", lua_tostring(L, -1));
    vm_release(L);
    return -1;
  }

  // Use the return value from the script as process exit code.
  res = 0;
  if (lua_type(L, -1) == LUA_TNUMBER) {
    res = (int)lua_tointeger(L, -1);
  }
  vm_release(L);
  return res;
}
