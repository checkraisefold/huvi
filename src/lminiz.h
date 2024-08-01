#ifndef LMINIZ_H
#define LMINIZ_H

static void lmz_reader_gc(void* zip);
static void lmz_writer_gc(void* zip);
static void lmz_inflator_gc(void* stream);
static void lmz_deflator_gc(void* stream);

#endif