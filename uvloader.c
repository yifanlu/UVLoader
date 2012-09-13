/**
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
 **/
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
uvl_start ()
{
    vita_init_log ();
    LOG ("UVLoader %u.%u.%u started.", UVL_VER_MAJOR, UVL_VER_MINOR, UVL_VER_REV);
    PsvUID uvl_thread;

    IF_DEBUG LOG ("Creating thread to run loader.");
    uvl_thread = sceKernelCreateThread ("uvloader", uvl_entry, 0x10000100, 0x00001000, 0, (0x01 << 16 | 0x02 << 16 | 0x04 << 16), NULL);
    if (uvl_thread < 0)
    {
        LOG ("Cannot create UVLoader thread.");
        return -1;
    }
    IF_DEBUG LOG ("Starting loader thread.");
    if (sceKernelStartThread (uvl_thread, 0, NULL) < 0)
    {
        LOG ("Cannot start UVLoader thread.");
        return -1;
    }
    IF_DEBUG LOG ("Removing old game thread.");
    if (sceKernelExitDeleteThread (0) < 0)
    {
        LOG ("Cannot delete current thread.");
        return -1;
    }
    // should not reach here
    IF_DEBUG LOG ("Error removing old thread.");
    return 0;
}

int
uvl_exit (int status)
{
    IF_DEBUG LOG ("Exit called. Status: 0x%08X", status);
    for(;;);
    return 0;
}

/********************************************//**
 *  \brief Entry point of UVLoader
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_entry ()
{
    int (*start)(int argc, char* argv);
    int ret_value;

    IF_DEBUG LOG ("Initializing resolve table.");
    if (uvl_resolve_table_initialize () < 0)
    {
        LOG ("Failed to initialize resolve table.");
        return -1;
    }
    IF_DEBUG LOG ("Filling resolve table.");
    if (uvl_resolve_add_all_modules (RESOLVE_MOD_IMPS | RESOLVE_MOD_EXPS | RESOLVE_IMPS_SVC_ONLY) < 0)
    {
        LOG ("Cannot cache all loaded entries.");
        return -1;
    }
#if 0
    IF_DEBUG LOG ("Adding custom exit() hook.");
    resolve_entry_t exit_resolve;
    exit_resolve.nid = 0x826BBBAF;
    exit_resolve.type = RESOLVE_TYPE_FUNCTION;
    exit_resolve.value.func_ptr = uvl_exit;
    if (uvl_resolve_table_add (&exit_resolve) < 0)
    {
        LOG ("Cannot add resolve for exit().");
        return -1;
    }
    IF_DEBUG LOG ("Exit at 0x%08X", exit_resolve.value.value);
#endif
    IF_DEBUG LOG ("Loading homebrew.");
    if (uvl_load_exe (UVL_HOMEBREW_PATH, (void**)&start) < 0)
    {
        LOG ("Cannot load homebrew.");
        return -1;
    }
    // sceKernelRegisterCallbackToEvent on exit
    // TODO: Free allocated memory and unload code
    IF_DEBUG LOG ("Running the homebrew.");
    ret_value = start (0, NULL);
    IF_DEBUG LOG ("Homebrew exited with value 0x%08X", ret_value);
    return 0;
}
