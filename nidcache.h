#ifndef UVL_NIDCACHE
#define UVL_NIDCACHE

#include "types.h"

typedef struct {
    u32_t module_nid;
    u32_t entries;
} nid_cache_db;

#if defined(FW_318)
#include "nidcache-318.h"
#elif defined(FW_315)
#include "nidcache-315.h"
#elif defined(FW_169)
#include "nidcache-169.h"
#else
#warning "No FW version specified, NID caching disabled"
#define LIBKERNEL_LIB_NID 0
const nid_cache_db libkernel_nid_cache_header[] = { {0, 0} };
const u32_t libkernel_nid_cache[0];
#endif

#endif
