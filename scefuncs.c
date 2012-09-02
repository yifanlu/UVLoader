#define GENERATE_STUBS
#include "resolve.h"
#include "scefuncs.h"

void uvl_scefuncs_resolve_all ()
{
    // First create a macro for adding stubs
    resolve_entry_t entry;
#define ADD_RESOLVE_STUB (stub, nid) \
    entry.nid = nid; \
    entry.type = RESOLVE_TYPE_FUNCTION; \
    entry.resolved = 0; \
    entry.value.func_ptr = stub;
    // no error checks
    uvl_resolve_table_add (&entry);

    // now add the stubs
    ADD_RESOLVE_STUB (sceIoOpen, 0xDEADBEEF);
    ADD_RESOLVE_STUB (sceIoClose, 0xDEADBEEF);

    // finally resolve all
    uvl_resolve_all_unresolved ();
}
