#define GENERATE_STUBS
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"

// old resolve table resolve code
#if 0
// There is no error check in this function. Hope code works.
void
uvl_scefuncs_resolve_all ()
{
    // First create a macro for adding stubs
    resolve_entry_t entry;
#define ADD_RESOLVE_STUB(_stub, _nid) \
    entry.nid = _nid; \
    entry.type = RESOLVE_TYPE_FUNCTION; \
    entry.resolved = 0; \
    entry.value.func_ptr = _stub; \
    uvl_resolve_table_add (&entry);

    // now add the stubs

    // make sure our macro isn't used elsewhere
#undef ADD_RESOLVE_STUB

    // finally resolve all
    // TODO: Resolve code
}
#endif

#if 0
// There is no error check in this function. Hope code works.
void
uvl_scefuncs_resolve_all ()
{
    // First create a macro for adding stubs
#define ADD_RESOLVE_STUB(_stub, _addr) \
    ((u32_t*)_stub)[0] = uvl_encode_arm_inst (INSTRUCTION_MOVW, (u16_t)(u32_t)_addr, 12); \
    ((u32_t*)_stub)[1] = uvl_encode_arm_inst (INSTRUCTION_MOVT, (u16_t)((u32_t)_addr >> 16), 12); \
    ((u32_t*)_stub)[2] = uvl_encode_arm_inst (INSTRUCTION_BRANCH, 0, 12);

    // now resolve the stubs
    psvUnlockMem ();
    ADD_RESOLVE_STUB(sceIoOpen, 0xE000AFE5);
    //ADD_RESOLVE_STUB(sceIoOpen, 0x82B275A5);
    ADD_RESOLVE_STUB(sceIoClose, 0xE0006224);
    //ADD_RESOLVE_STUB(sceIoClose, 0x82B275AD);
    ADD_RESOLVE_STUB(sceIoLseek, 0xE000AFF5);
    //ADD_RESOLVE_STUB(sceIoLseek, 0x82B275DD);
    ADD_RESOLVE_STUB(sceIoRead, 0xE00062B4);
    //ADD_RESOLVE_STUB(sceIoRead, 0x82B275CD);
    ADD_RESOLVE_STUB(sceKernelGetModuleList, 0xE0005F94);
    ADD_RESOLVE_STUB(sceKernelGetModuleInfo, 0xE0005FA4);
    ADD_RESOLVE_STUB(sceKernelCreateThread, 0xE000B6B9);
    ADD_RESOLVE_STUB(sceKernelStartThread, 0xE000B291);
    ADD_RESOLVE_STUB(sceKernelExitDeleteThread, 0xE0005A54);
    ADD_RESOLVE_STUB(sceKernelStopUnloadModule, 0xE00011F9);
    ADD_RESOLVE_STUB(sceKernelAllocMemBlock, 0xE0040570);
    ADD_RESOLVE_STUB(sceKernelAllocCodeMemBlock, 0x82B39BD0);
    ADD_RESOLVE_STUB(sceKernelGetMemBlockBase, 0xE0040560);
    ADD_RESOLVE_STUB(sceKernelFreeMemBlock, 0xE0040550);
    psvLockMem ();

    // make sure our macro isn't used elsewhere
#undef ADD_RESOLVE_STUB
}
#endif
