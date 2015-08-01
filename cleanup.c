/*
 * cleanup.c - Frees used memory by the game
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

#include "uvloader.h"
#include "cleanup.h"
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"

static u32_t unity_player_seg1 = 0x0;
static int unity_version = 0x0;

typedef u32_t (*cleanup_graphics_func)(void*);
static cleanup_graphics_func cleanup_graphics;
static int graphics_finished_cleaning = 0;

typedef u32_t (*sceCtrlPeekBufferPositive_func)(int, u32_t*, int);
static sceCtrlPeekBufferPositive_func sceCtrlPeekBufferPositive_syscall;

/********************************************//**
 *  \brief Convert L and R button values to what is expected.
 ***********************************************/
u32_t
uvl_wrapper_sceCtrlPeekBufferPositive(int port,
u32_t *pad_data,
int count)
{
    int res = sceCtrlPeekBufferPositive_syscall(port, pad_data, count);
    pad_data[2] |= (pad_data[2] & 0xF00) >> 2; // 0x400 (L) becomes 0x100 and 0x800 (R) becomes 0x200.
    pad_data[2] &= ~0xC00;

    return res;
}

/********************************************//**
 *  \brief Add the resolved function to the table.
 ***********************************************/
void
uvl_add_func_by_ptr(u32_t nid,
u16_t type,
void* func_ptr)
{
    resolve_entry_t resolve_entry;
    resolve_entry.nid = nid;
    resolve_entry.type = type;
    resolve_entry.value.func_ptr = func_ptr;

    uvl_resolve_table_add(&resolve_entry);
}


/********************************************//**
 *  \brief Check the Unity modules.
 *  
 *  Manually resolve functions from the modules.
 ***********************************************/
int
uvl_cleanup_check_module(PsvUID modid, ///< UID of the module
int index)  ///< An OR combination of flags (see defined "Search flags for importing loaded modules") directing the search
{
    loaded_module_info_t m_mod_info;
    m_mod_info.size = sizeof (loaded_module_info_t); // should be 440

    if (sceKernelGetModuleInfo(modid, &m_mod_info) < 0)
    {
        LOG("Error getting info for mod 0x%08X", modid);
        return -1;
    }
    
    if (strcmp(m_mod_info.module_name, "UnityPlayer") == 0)
    {
        uvl_unlock_mem();

        unity_version = 0x105;    // For now, it is assumed that this is version 1.05 since 1.06 has a different module name for UnityPlayer...
        unity_player_seg1 = (u32_t)m_mod_info.segments[1].vaddr;

        u32_t unitybaseseg0 = (u32_t) m_mod_info.segments[0].vaddr;

        uvl_add_func_by_ptr(0x5795E898, RESOLVE_TYPE_FUNCTION, (void*) (unitybaseseg0 + 0x9EEC6C)); // sceDisplayWaitVblankStart
        uvl_add_func_by_ptr(0xFF082DF0, RESOLVE_TYPE_FUNCTION, (void*) (unitybaseseg0 + 0x9EECCC)); // sceTouchPeek
        uvl_add_func_by_ptr(0xA9C3CED6, RESOLVE_TYPE_FUNCTION, (void*) (unitybaseseg0 + 0x9EEC8C)); // sceCtrlPeekBufferPositive

        uvl_lock_mem();
    }
    else if (strcmp(m_mod_info.module_name, "UnityPlayer_4370_Develop") == 0)
    {
        uvl_unlock_mem();

        unity_version = 0x106;
        unity_player_seg1 = (u32_t)m_mod_info.segments[1].vaddr;

        u32_t unitybaseseg0 = (u32_t) m_mod_info.segments[0].vaddr;

        uvl_add_func_by_ptr(0x5795E898, RESOLVE_TYPE_FUNCTION, (void*) (unitybaseseg0 + 0x9E91BC)); // sceDisplayWaitVblankStart
        uvl_add_func_by_ptr(0xFF082DF0, RESOLVE_TYPE_FUNCTION, (void*) (unitybaseseg0 + 0x9E922C)); // sceTouchPeek

        void* sceCtrlPeekBufferPositive_ptr = &uvl_wrapper_sceCtrlPeekBufferPositive;
        uvl_add_func_by_ptr(0xA9C3CED6, RESOLVE_TYPE_FUNCTION, (void*) (sceCtrlPeekBufferPositive_ptr)); // sceCtrlPeekBufferPositive
        sceCtrlPeekBufferPositive_syscall = (sceCtrlPeekBufferPositive_func) (unitybaseseg0 + 0x9E91CC);

        uvl_lock_mem();
    }

    return 0;
}

/********************************************//**
 *  \brief Hook for the Unity graphics thread
 *  
 *  Call the Unity graphics class destructor.
 ***********************************************/
void
uvl_cleanup_graphics_thread_hook(void* r0)
{
    IF_DEBUG LOG("Hooked the Unity graphics thread.");

    cleanup_graphics(r0);
    
    uvl_unlock_mem();
    graphics_finished_cleaning = 1;
    uvl_lock_mem();

    sceKernelExitDeleteThread(0);
}

/********************************************//**
 *  \brief Clean up Unity
 *  
 *  Check if Unity PSM is running and cleans it.
 ***********************************************/
void
uvl_clean_unity()
{
    PsvUID mod_list[MAX_LOADED_MODS];
    u32_t num_loaded = MAX_LOADED_MODS;

    if (sceKernelGetModuleList(0xFF, mod_list, &num_loaded) < 0)
    {
        LOG("Failed to get module list.");
        return;
    }

    int i;
    for (i = 0; i < num_loaded; i++)
    {
        if (uvl_cleanup_check_module(mod_list[i], i) < 0)
        {
            LOG("Failed to add module %u: 0x%08X. Continuing.", i, mod_list[i]);
            continue;
        }
    }
    
    if (unity_version != 0x0)
    {
        IF_DEBUG LOG("Unity Version: 0x%x", unity_version);

        u32_t cleanup_hook_ptr = (u32_t) &uvl_cleanup_graphics_thread_hook;
        u32_t new_graphics_class_vtable[250];

        int i;
        for (i = 0; i < 250; ++i)
        {
            new_graphics_class_vtable[i] = cleanup_hook_ptr;
        }

        u32_t* graphics_class = *(u32_t**) (unity_player_seg1 + 0xEB8);
        u32_t* graphics_class_vtable = (u32_t*) *graphics_class;
        
        uvl_unlock_mem();
        cleanup_graphics = (cleanup_graphics_func) (*(graphics_class_vtable + 1));
        uvl_lock_mem();

        // Overwrite the graphics class vtable. At some point after doing this, the graphics thread will call 'cleanup_graphics'.
        *graphics_class = (u32_t)new_graphics_class_vtable;

        // Give the graphics thread time to be hooked and exit.
        while (graphics_finished_cleaning == 0)
        {
            // sceKernelDelayThread should be used here, but it doesn't seem to work.
        }

        IF_DEBUG LOG("Finished cleaning Unity");
    }
}

/********************************************//**
 *  \brief Clean up used resources.
 ***********************************************/
void
uvl_pre_clean ()
{
    uvl_clean_unity();
}

/********************************************//**
 *  \brief Free up the RAM
 *  
 *  Frees up memory used by the loaded game.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_cleanup_memory ()
{
    // close and delete all threads
    if (uvl_unload_all_modules () < 0)
    {
        LOG ("Failed to unload all modules.");
        return -1;
    }
    // free allocated memory
    // delete event flags, semaphors, mutexs, etc
    // close all file handles
}

/********************************************//**
 *  \brief Unloads loaded modules
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_unload_all_modules ()
{
    loaded_module_info_t m_mod_info;
    PsvUID mod_list[MAX_LOADED_MODS];
    u32_t num_loaded = MAX_LOADED_MODS;
    int status;
    int i;

    if (sceKernelGetModuleList (0xFF, mod_list, &num_loaded) < 0)
    {
        LOG ("Failed to get module list.");
        return -1;
    }
    for (i = 0; i < num_loaded; i++)
    {
       if (sceKernelStopUnloadModule (mod_list[i], 0, NULL, &status, NULL) < 0)
       {
            LOG ("Failed to unload module %X, continuing...", mod_list[i]);
            continue;
       }
    }
    return 0;
}