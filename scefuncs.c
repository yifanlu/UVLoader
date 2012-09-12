/**
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
 **/
#define GENERATE_STUBS
#include "resolve.h"
#include "scefuncs.h"

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
