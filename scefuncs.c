#define GENERATE_STUBS
#include "resolve.h"
#include "scefuncs.h"

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
