/*
 * scefuncs.c - Hooks for API functions
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
#define GENERATE_STUBS
#include "config.h"
#include "resolve.h"
#include "scefuncs.h"

/********************************************//**
 *  \brief Resolves UVLoader
 *  
 *  This function will try its best to resolve 
 *  imported function given their NIDs. UVL 
 *  only uses sceLibKernel API calls which 
 *  should be imported by every game.
 ***********************************************/
void
uvl_scefuncs_resolve_loader ()
{
    // we must first find the relocated sceLibKernel base
    // by resolving the first 
    resolve_entry_t kernel_base;
    // unfortunally, we can't do error checks yet so let's pray
    // it doesn't crash
    uvl_resolve_import_stub_to_entry ((void*)UVL_RESOLVE_PBASE, 0, &kernel_base);
    kernel_base.value.value &= (~0 - 1); // unset first bit (entry is thumb code)
    #define RESOLVE_STUB(_stub, _nid) uvl_resolve_loader (_nid, kernel_base.value.ptr, _stub);

    RESOLVE_STUB(sceKernelStopUnloadModule, 0x2415F8A4);
    RESOLVE_STUB(sceKernelFindMemBlockByAddr, 0xA33B99D1);
    RESOLVE_STUB(sceKernelFreeMemBlock, 0xA91E15EE);
    RESOLVE_STUB(sceKernelGetMemBlockBase, 0xB8EF5818);
    RESOLVE_STUB(sceKernelAllocMemBlock, 0xB9D5EBDE);
    RESOLVE_STUB(sceKernelExitDeleteThread, 0x1D17DECF);
    RESOLVE_STUB(sceKernelGetModuleList, 0x2EF2581F);
    RESOLVE_STUB(sceKernelGetModuleInfo, 0x36585DAF);
    RESOLVE_STUB(sceIoWrite, 0x34EFD876);
    RESOLVE_STUB(sceIoClose, 0xC70B8886);
    RESOLVE_STUB(sceIoRead, 0xFDB32293);
    RESOLVE_STUB(sceIoOpen, 0x6C60AC61);
    RESOLVE_STUB(sceKernelStartThread, 0xF08DE149);
    RESOLVE_STUB(sceKernelCreateThread, 0xC5C11EE7);

    #undef RESOLVE_STUB
}
