#include "cleanup.h"
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"

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

int
uvl_unload_all_modules ()
{
    loaded_module_info_t m_mod_info;
    SceUID mod_list[MAX_LOADED_MODS];
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
