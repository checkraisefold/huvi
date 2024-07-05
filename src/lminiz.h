#ifndef LMINIZ_H
#define LMINIZ_H

#include "lua.h"
#include "lualib.h"

static void lmz_reader_gc(lua_State* L);
static void lmz_writer_gc(lua_State* L);
static void lmz_inflator_gc(lua_State* L);
static void lmz_deflator_gc(lua_State* L);

#endif