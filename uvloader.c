#include "cleanup.h"
#include "load.h"
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"
#include "uvloader.h"

/********************************************//**
 *  \brief Entry point from exploit
 *  
 *  Call this from your exploit to run UVLoader.
 *  It will first cache all loaded modules and 
 *  attempt to resolve its own NIDs which 
 *  should only depend on sceLibKernel.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int START_SECTION
uvl_entry ()
{
    SceUID uvl_thread;
    // TODO: find a place in memory to store table.
    if (uvl_resolve_all_loaded_modules (RESOLVE_MOD_IMPS | RESOLVE_MOD_EXPS | RESOLVE_IMPS_SVC_ONLY) < 0)
    {
        LOG ("Cannot cache all loaded modules.");
        return -1;
    }
    if (uvl_add_unresolved_uvloader () < 0)
    {
        LOG ("Cannot find UVLoader stubs.");
        return -1;
    }
    if (uvl_resolve_all_unresolved () < 0)
    {
        LOG ("Cannot resolve UVLoader stubs.");
        return -1;
    }
    uvl_thread = sceKernelCreateThread ("uvloader", uvl_start, 0x18, 0x10000, 0, NULL);
    if (uvl_thread < 0)
    {
        LOG ("Cannot create UVLoader thread.");
        return -1;
    }
    if (sceKernelStartThread (uvl_thread, 0, NULL) < 0)
    {
        LOG ("Cannot start UVLoader thread.");
        return -1;
    }
    if (sceKernelExitDeleteThread (0) < 0)
    {
        LOG ("Cannot delete current thread.");
        return -1;
    }
    // should not reach here
    return 0;
}

/********************************************//**
 *  \brief Starts UVLoader
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_start ()
{
    // clean up ram
    // load ELF
}
