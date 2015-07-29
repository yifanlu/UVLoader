/*
 * uvloader.c - Userland Vita Loader entry point
 * Copyright 2012 Yifan Lu
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *    http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "cleanup.h"
#include "config.h"
#include "load.h"
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"
#include "uvloader.h"

// make sure code is PIE
#ifndef __PIE__
#error "Must compile with -fPIE"
#endif

static uvl_context_t *g_context;

static void uvl_init_from_context (uvl_context_t *ctx);

/********************************************//**
 *  \brief Starting point from exploit
 *  
 *  Call this from your exploit to run UVLoader.
 *  It will first cache all loaded modules and 
 *  attempt to resolve its own NIDs which 
 *  should only depend on sceLibKernel.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int START_SECTION
uvl_start (uvl_context_t *ctx)    ///< Pass in context information
{
    uvl_init_from_context (ctx);
    uvl_scefuncs_resolve_loader (ctx->libkernel_anchor);
    vita_init_log ();
    LOG ("UVLoader %u.%u.%u started.", UVL_VER_MAJOR, UVL_VER_MINOR, UVL_VER_REV);
    return uvl_start_load ();
}

/********************************************//**
 *  \brief Sets up hooks from context buffer
 ***********************************************/
static void
uvl_init_from_context (uvl_context_t *ctx)  ///< Context buffer
{
    ctx->psvUnlockMem ();
    g_context = ctx;
    ctx->psvLockMem ();
}

/********************************************//**
 *  \brief From context buffer
 ***********************************************/
void *
uvl_alloc_code_mem (unsigned int *p_len)
{
    return g_context->psvCodeAllocMem (p_len);
}

/********************************************//**
 *  \brief From context buffer
 ***********************************************/
void
uvl_unlock_mem (void)
{
    g_context->psvUnlockMem ();
}

/********************************************//**
 *  \brief From context buffer
 ***********************************************/
void
uvl_lock_mem (void)
{
    g_context->psvLockMem ();
}

/********************************************//**
 *  \brief From context buffer
 ***********************************************/
void
uvl_flush_icache (void *addr, unsigned int len)
{
    g_context->psvFlushIcache (addr, len);
}

/********************************************//**
 *  \brief From context buffer
 ***********************************************/
int
uvl_debug_log (const char *line)
{
    if (g_context->logline)
        return g_context->logline (line);
    else
        return -1;
}

/********************************************//**
 *  \brief printf proxy
 *  
 *  Used for exporting debug logging
 ***********************************************/
static int
printf (const char *format, ...)
{
    char buffer[MAX_LOG_LENGTH];
    va_list arg;

    va_start (arg, format);
    sceClibVsnprintf (buffer, MAX_LOG_LENGTH, format, arg);
    va_end (arg);

    uvl_debug_log (buffer);
    return 0;
}

/********************************************//**
 *  \brief Add custom NID entries for UVL
 ***********************************************/
void
uvl_add_uvl_exports (void)
{
    resolve_entry_t entry;
    entry.nid = UVL_EXIT_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = uvl_exit;
    uvl_resolve_table_add (&entry);
    entry.nid = UVL_CODE_ALLOC_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = uvl_alloc_code_mem;
    uvl_resolve_table_add (&entry);
    entry.nid = UVL_CODE_UNLOCK_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = uvl_unlock_mem;
    uvl_resolve_table_add (&entry);
    entry.nid = UVL_CODE_LOCK_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = uvl_lock_mem;
    uvl_resolve_table_add (&entry);
    entry.nid = UVL_CODE_FLUSH_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = uvl_flush_icache;
    uvl_resolve_table_add (&entry);
    entry.nid = UVL_DEBUG_LOG_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = uvl_debug_log;
    uvl_resolve_table_add (&entry);
    entry.nid = UVL_PRINTF_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = printf;
    uvl_resolve_table_add (&entry);
    entry.nid = UVL_LOAD_NID;
    entry.type = RESOLVE_TYPE_FUNCTION;
    entry.value.func_ptr = uvl_load;
    uvl_resolve_table_add (&entry);
}

/********************************************//**
 *  \brief Main function. Builds resolves and load. 
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_start_load ()
{
    IF_DEBUG LOG ("Initializing resolve table.");
    if (uvl_resolve_table_initialize () < 0)
    {
        LOG ("Failed to initialize resolve table.");
        return -1;
    }
    IF_DEBUG LOG ("Filling resolve table.");
    if (uvl_resolve_add_all_modules (RESOLVE_MOD_IMPS | RESOLVE_MOD_EXPS | RESOLVE_IMPS_SVC_ONLY | RESOLVE_RELOAD_MOD) < 0)
    {
        LOG ("Cannot cache all loaded entries.");
        return -1;
    }
    IF_DEBUG LOG ("Adding UVL imports.");
    uvl_add_uvl_exports ();
    
    uvl_pre_clean();
    
    IF_DEBUG LOG ("Loading homebrew.");
    if (uvl_load (UVL_HOMEBREW_PATH) < 0)
    {
        return -1;
    }
    IF_DEBUG LOG ("Cleaning up.");
    uvl_resolve_table_destroy ();
    return 0;
}

/********************************************//**
 *  \brief Loads a executable at path
 *  
 *  Starts a new thread with the homebrew 
 *  loaded. Waits until the homebrew exits and 
 *  returns.
 *  \returns Thread ID on success, otherwise error
 ***********************************************/
int 
uvl_load (const char *path)
{
    char data_blob[LOADED_INFO_SIZE];
    uvl_loaded_t *loaded;
    int (*start)(int, void *);
    int ret_value;
    PsvUID tid;
    int i;

    loaded = (uvl_loaded_t *)data_blob;
    IF_DEBUG LOG ("Loading homebrew");
    if (uvl_load_exe (path, (void**)&start, loaded) < 0)
    {
        LOG ("Cannot load homebrew.");
        return -1;
    }
    IF_DEBUG LOG ("Starting homebrew: entry at 0x%08X", start);
    tid = sceKernelCreateThread ("homebrew", start, 0, 0x00040000, 0, 0x00070000, NULL);
    if (tid < 0)
    {
        LOG ("Cannot create thread.");
        return -1;
    }
    loaded->tid = tid;
    IF_DEBUG LOG ("Starting new thread.");
    if (sceKernelStartThread (tid, 0, NULL) < 0)
    {
        LOG ("Cannot start thread.");
        return -1;
    }
    IF_DEBUG LOG ("Homebrew running...");
    if (sceKernelWaitThreadEnd (tid, &ret_value, NULL) < 0)
    {
        LOG ("Failed to wait for thread to exit.");
    }
    IF_DEBUG LOG ("Homebrew exited with value 0x%08X", ret_value);

    // free segments
    for (i = 0; i < loaded->numsegs; i++)
    {
        sceKernelFreeMemBlock (loaded->segs[i]);
    }
    
    return 0;
}

/********************************************//**
 *  \brief Exiting point for loaded application
 *  
 *  This hooks on to exit() call and cleans up 
 *  after the application is unloaded.
 *  \returns Zero on success, otherwise error
 ***********************************************/
void
uvl_exit (int status)
{
    sceKernelExitDeleteThread (0);
}
