#ifndef LMINIZ_H
#define LMINIZ_H

#include "lua.h"
#include "lualib.h"

static void lmz_reader_gc(lmz_file_t* zip);
static void lmz_writer_gc(lmz_file_t* zip);
static void lmz_inflator_gc(lmz_stream_t* stream);
static void lmz_deflator_gc(lmz_stream_t* stream);

#endif