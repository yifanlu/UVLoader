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
uvl_start (void *anchor)    ///< Pass in pointer to an SceLibKernel import to defeat ASLR
{
    uvl_scefuncs_resolve_loader (anchor); // must be first
    vita_init_log ();
    LOG ("UVLoader %u.%u.%u started.", UVL_VER_MAJOR, UVL_VER_MINOR, UVL_VER_REV);
#ifdef UVL_NEW_THREAD
    PsvUID uvl_thread;
    IF_DEBUG LOG ("Creating thread to run loader.");
    uvl_thread = sceKernelCreateThread ("uvloader", uvl_entry, 0, 0x00040000, 0, 0x00070000, NULL);
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
    IF_DEBUG LOG ("Error removing thread.");
    return -1;
#else
    return uvl_entry ();
#endif
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
    IF_DEBUG LOG ("Adding custom exit() hook.");
    resolve_entry_t exit_resolve = { EXIT_NID, RESOLVE_TYPE_FUNCTION, 0, uvl_exit };
    if (uvl_resolve_table_add (&exit_resolve) < 0)
    {
        LOG ("Cannot add resolve for exit().");
        return -1;
    }
    IF_DEBUG LOG ("Exit at 0x%08X", exit_resolve.value.value);
    IF_DEBUG LOG ("Loading homebrew.");
    if (uvl_load_exe (UVL_HOMEBREW_PATH, (void**)&start) < 0)
    {
        LOG ("Cannot load homebrew.");
        return -1;
    }
    IF_DEBUG LOG ("Freeing resolve table.");
    if (uvl_resolve_table_destroy () < 0)
    {
        LOG ("Cannot destroy resolve table.");
        return -1;
    }
    IF_DEBUG LOG ("Running the homebrew.");
    ret_value = start (0, NULL);
    // should not reach here
    IF_DEBUG LOG ("Homebrew exited with value 0x%08X", ret_value);
    return 0;
}

/********************************************//**
 *  \brief Exiting point for loaded application
 *  
 *  This hooks on to exit() call and cleans up 
 *  after the application is unloaded.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_exit (int status)
{
    IF_DEBUG LOG ("Exit called with status: 0x%08X", status);
    IF_DEBUG LOG ("Removing application thread.");
    if (sceKernelExitDeleteThread (0) < 0)
    {
        LOG ("Cannot delete application thread.");
        return -1;
    }
    // should not reach here
    IF_DEBUG LOG ("Error removing thread.");
    return 0;
}
