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

    // make sure our macro isn't used elsewhere
#undef ADD_RESOLVE_STUB
}
#endif
