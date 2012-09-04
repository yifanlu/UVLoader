#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"

/** Checks if a bit is set in a given member */
#define BIT_SET(i, b) (i & (0x1 << b))
/** Checks if this is a valid ARM address */
#define PTR_VALID(ptr) (ptr > 0x81000000 && ptr < 0xF0000000) // TODO: change this to be better 
/** The first entry in the resolve table containing information to resolve NIDs */
resolve_entry_t *RESOLVE_TABLE = (void*)0x85000000;
/** Number of entries in the resolve table */
int RESOLVE_ENTRIES = 0;

/********************************************//**
 *  \brief Adds a resolve entry
 *  
 *  This function does not check for duplicates.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_resolve_table_add (resolve_entry_t *entry) ///< Entry to add
{
    void *location = &RESOLVE_TABLE[RESOLVE_ENTRIES];
    memcpy (location, entry, sizeof (resolve_entry_t));
    RESOLVE_ENTRIES++;
    return 0;
}

/********************************************//**
 *  \brief Gets a resolve entry
 *  
 *  This function returns the first entry in the 
 *  table that has the given NID. If 
 *  @a force_resolved is used, it will not 
 *  return unresolved entries.
 *  \returns Entry on success, NULL on error
 ***********************************************/
resolve_entry_t *
uvl_resolve_table_get (u32_t nid,               ///< NID to resolve
                         int forced_resolved)   ///< Only return resolved entries
{
    int i;
    for (i = 0; i < RESOLVE_ENTRIES; i++)
    {
        if (RESOLVE_TABLE[i].nid == nid)
        {
            // entries could be placeholders for resolving later
            if (forced_resolved && !RESOLVE_TABLE[i].resolved)
            {
                continue;
            }
            return &RESOLVE_TABLE[i];
        }
    }
    return NULL;
}

// TODO: Implement this
/********************************************//**
 *  \brief Estimates an unknown syscall
 *  
 *  Estimates a syscall for a given NID based 
 *  on information of existing syscalls in the 
 *  resolve table.
 *  \returns Entry on success, NULL on error
 ***********************************************/
resolve_entry_t *
uvl_estimate_syscall (u32_t nid) ///< NID to resolve
{
    return NULL;
}

/********************************************//**
 *  \brief Creates resolve entry from stub 
 *  function imported by a module
 *  
 *  Given the stub function of a loaded module 
 *  that has already been resolved, extract 
 *  the resolved information and write it to 
 *  a @c resolve_entry_t.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_import_stub_to_entry (void *func,  ///< Stub function to read
                         u32_t nid,    ///< NID resolved by stub
               resolve_entry_t *entry) ///< Entry to write to
{
    u32_t val = 0;
    u8_t inst_type = 0;
    int read = 0;
    entry->type = RESOLVE_TYPE_UNKNOWN;
    for (;;)
    {
        val = uvl_decode_arm_inst (*(u32_t*)func, &inst_type);
        /*
        if (val < 0)
        {
            // instruction invalid, hopefully we got what we need
            break;
        }
        if (val == 0)
        {
            // something went wrong, either this imported stub was not filled
            // or we screwed up
            continue;
        }
        */
        switch (inst_type)
        {
            case INSTRUCTION_MOV:
                entry->value.value = val;
                break;
            case INSTRUCTION_MOVT:
                entry->value.value |= val << 16; // load upper
                break;
            case INSTRUCTION_SYSCALL:
                entry->type = RESOLVE_TYPE_SYSCALL; // value's meaning changed to syscall
                break;
            case INSTRUCTION_BRANCH:
                entry->type = RESOLVE_TYPE_FUNCTION; // value's meaning changed to function ptr
                break;
            case INSTRUCTION_UNKNOWN: // we should have already breaked!
            default:
                break;
        }
        if (entry->type != RESOLVE_TYPE_UNKNOWN && entry->value.value > 0)
        {
            // we resolved it, get out before something weird happens
            entry->resolved = 1;
            break;
        }
        // next instruction
        func = (char*)func + sizeof (u32_t);
    }
    if (!entry->resolved) // we failed :(
    {
        LOG ("Failed to resolve import NID: 0x%08X", nid);
        return -1;
    }
    // put the finishing touches
    entry->nid = nid;
    return 0;
}

/********************************************//**
 *  \brief Get an instruction's type and
 *  immediate value
 *  
 *  Currently only supports ARMv7 encoding of 
 *  MOV Rd, \#imm, MOVT Rd, \#imm, BX Rn, and 
 *  SVC \#imm. You should use the return value 
 *  of this function to check for error but 
 *  instead the value of @a type where 
 *  @c INSTRUCTION_UNKNOWN is a failure.
 *  \returns Immediate value of instruction
 *  on success, indeterminate on error
 ***********************************************/
u32_t 
uvl_decode_arm_inst (u32_t cur_inst, ///< ARMv7 instruction
                      u8_t *type)    ///< See defined "Supported ARM instruction types"
{
    // if this doesn't change, error
    *type = INSTRUCTION_UNKNOWN;
    // Bits 31-28 should be 1110, Always Execute
    if (!(BIT_SET (cur_inst, 31) && BIT_SET (cur_inst, 30) && BIT_SET (cur_inst, 29) && !BIT_SET (cur_inst, 28)))
    {
        // Unsupported conditional instruction.
        return -1;
    }
    // Bits 27-25 should be 110 for supervisor calls
    if (BIT_SET (cur_inst, 27))
    {
        // Bit 24 should be set for SWI calls
        if (BIT_SET (cur_inst, 26) && !BIT_SET (cur_inst, 25) && BIT_SET (cur_inst, 24))
        {
            *type = INSTRUCTION_SYSCALL;
            // TODO: Return syscall immediate value.
            return 1;
        }
    }
    // Bits 27-25 should be 001 for data instructions
    else if (!BIT_SET (cur_inst, 26) && BIT_SET (cur_inst, 25))
    {
        // Bits 24-23 should be 10
        if (!(BIT_SET (cur_inst, 24) && !BIT_SET (cur_inst, 23)))
        {
            // Not an valid ARM MOV instruction.
            return -1;
        }
        // Bits 21-20 should be 00
        if (!(!BIT_SET (cur_inst, 21) && !BIT_SET (cur_inst, 20)))
        {
            // Invalid ARM MOV instruction.
            return -1;
        }
        // Bits 15-12 should be 1100 for R12 load
        if (!(BIT_SET (cur_inst, 15) && BIT_SET (cur_inst, 14) && !BIT_SET (cur_inst, 13) && !BIT_SET (cur_inst, 12)))
        {
            // Not R12, unsupported
            return -1;
        }
        // Bit 22 is 1 for top load 0 for bottom load
        if (BIT_SET (cur_inst, 22)) // top load
        {
            *type = INSTRUCTION_MOVT;
        }
        else
        {
            *type = INSTRUCTION_MOV;
        }
        // Immediate value at 19-16 and 11-0
        // discard bytes 31-20
        // discard bytes 15-0
        return (((cur_inst << 12) >> 28) << 12) | ((cur_inst << 20) >> 20);
    }
    // Bits 27-25 should be 000 for BLX instructions
    else if (!BIT_SET (cur_inst, 26) && !BIT_SET (cur_inst, 25))
    {
        // Bits 24-4 should be 101111111111110001, 0x12FFF1
        if ((cur_inst << 7) >> 15 != 0x2FFF1)
        {
            // Not a BX Rn instruction
            return -1;
        }
        else
        {
            *type = INSTRUCTION_BRANCH;
            return 1;
        }
    }
    else
    {
        // Unsupported instruction.
        return -1;
    }
}

/********************************************//**
 *  \brief Creates an ARMv7 instruction
 *  
 *  Currently only supports ARMv7 encoding of 
 *  MOV Rd, \#imm, MOVT Rd, \#imm, BX Rn, and 
 *  SVC \#imm. Depending on the @a type, not 
 *  all parameters may be used.
 *  \returns ARMv7 instruction on success 
 *  or 0 on failure.
 ***********************************************/
u32_t 
uvl_encode_arm_inst (u8_t type,  ///< See defined "Supported ARM instruction types"
                    u16_t immed, ///< Immediate value to encode
                    u16_t reg)   ///< Register used in instruction
{
    switch(type)
    {
        case INSTRUCTION_MOV:
            // 1110 0011 0000 XXXX YYYY XXXXXXXXXXXX
            // where X is the immediate and Y is the register
            // Upper bits == 0xE30
            return ((u32_t)0xE30 << 20) | ((u32_t)((immed >> 12) << 12) << 4) | ((immed << 4) >> 4) | (reg << 12);
        case INSTRUCTION_MOVT:
            // 1110 0011 0100 XXXX YYYY XXXXXXXXXXXX
            // where X is the immediate and Y is the register
            // Upper bits == 0xE34
            return ((u32_t)0xE34 << 20) | ((u32_t)((immed >> 12) << 12) << 4) | ((immed << 4) >> 4) | (reg << 12);
        case INSTRUCTION_SYSCALL:
            // Syscall does not have any immediate value, the number should
            // already be in R12
            return (u32_t)0xEF000000;
        case INSTRUCTION_BRANCH:
            // 1110 0001 0010 111111111111 0001 YYYY
            // BX Rn has 0xE12FFF1 as top bytes
            return ((u32_t)0xE12FFF1 << 4) | reg;
        case INSTRUCTION_UNKNOWN:
        default:
            return 0;
    }
}

/********************************************//**
 *  \brief Add a module's import table's entries 
 *  to the resolve table
 *  
 *  A loaded module has it's stubs already 
 *  resolved by the kernel. This will copy 
 *  those resolved entries for our own use.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_add_resolved_imports (module_imports_t *imp_table,    ///< Module's import table to read from
                                       int syscalls_only) ///< If set, will only add resolved syscalls and nothing else
{
    // this should be called BEFORE cleanup
    resolve_entry_t res_entry;
    u32_t *memory;
    u32_t nid;
    int i;
    // get functions first
    for(i = 0; i < imp_table->num_functions; i++)
    {
        nid = imp_table->func_nid_table[i];
        memory = imp_table->func_entry_table[i];
        if (uvl_import_stub_to_entry (memory, nid, &res_entry) < 0)
        {
            LOG ("Error generating entry from import stub.");
            return -1;
        }
        if (syscalls_only && res_entry.type != RESOLVE_TYPE_SYSCALL)
        {
            continue;
        }
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    if (syscalls_only)
    {
        return 0;
    }
    // get variables
    res_entry.type = RESOLVE_TYPE_VARIABLE;
    res_entry.resolved = 1;
    for(i = 0; i < imp_table->num_vars; i++)
    {
        res_entry.nid = imp_table->var_nid_table[i];
        res_entry.value.value = *(u32_t*)imp_table->var_entry_table[i];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    // get TLS
    // TODO: Find out how this works
    res_entry.type = RESOLVE_TYPE_VARIABLE;
    res_entry.resolved = 1;
    for(i = 0; i < imp_table->num_tls_vars; i++)
    {
        res_entry.nid = imp_table->tls_nid_table[i];
        res_entry.value.value = *(u32_t*)imp_table->tls_entry_table[i];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    return 0;
}

/********************************************//**
 *  \brief Add the homebrew's unresolved entries 
 *  to the resolve table
 *  
 *  Reads a homebrew's import table and adds 
 *  the entries into the resolve table. A later 
 *  call to \sa uvl_resolve_all_unresolved will
 *  resolve them.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_add_unresolved_imports (module_imports_t *imp_table) ///< Homebrew's import table
{
    resolve_entry_t res_entry;
    int i;
    res_entry.resolved = 0;


    // get functions first
    res_entry.type = RESOLVE_TYPE_FUNCTION;
    for(i = 0; i < imp_table->num_functions; i++)
    {
        res_entry.nid = imp_table->func_nid_table[i];
        res_entry.value.func_ptr = imp_table->func_entry_table[i];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    // get variables
    res_entry.type = RESOLVE_TYPE_VARIABLE;
    for(i = 0; i < imp_table->num_vars; i++)
    {
        res_entry.nid = imp_table->var_nid_table[i];
        res_entry.value.ptr = imp_table->var_entry_table[i];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    // get TLS
    // TODO: Find out how this works
    res_entry.type = RESOLVE_TYPE_VARIABLE;
    for(i = 0; i < imp_table->num_tls_vars; i++)
    {
        res_entry.nid = imp_table->tls_nid_table[i];
        res_entry.value.ptr = imp_table->tls_entry_table[i];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    return 0;
}

/********************************************//**
 *  \brief Add a module's export entries to 
 *  the resolve table
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_add_resolved_exports (module_exports_t *exp_table) ///< Module's export table
{
    resolve_entry_t res_entry;
    int i;
    int offset = 0;
    res_entry.resolved = 1;

    // get functions first
    res_entry.type = RESOLVE_TYPE_FUNCTION;
    for(i = 0; i < exp_table->num_functions; i++, offset++)
    {
        res_entry.nid = exp_table->nid_table[offset];
        res_entry.value.func_ptr = exp_table->entry_table[offset];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    // get variables
    res_entry.type = RESOLVE_TYPE_VARIABLE;
    for(i = 0; i < exp_table->num_vars; i++, offset++)
    {
        res_entry.nid = exp_table->nid_table[offset];
        res_entry.value.value = *(u32_t*)exp_table->entry_table[offset];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    // get TLS
    // TODO: Find out how this works
    res_entry.type = RESOLVE_TYPE_VARIABLE;
    for(i = 0; i < exp_table->num_tls_vars; i++, offset++)
    {
        res_entry.nid = exp_table->nid_table[offset];
        res_entry.value.value = *(u32_t*)exp_table->entry_table[offset];
        if (uvl_resolve_table_add (&res_entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    return 0;
}

/********************************************//**
 *  \brief Walks the resolve table, locates 
 *  unresolved entries and resolves them
 *  
 *  Called after building the resolve table from 
 *  resolved entries of loaded modules and 
 *  unresolved entries of the homebrew.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_resolve_all_unresolved ()
{
    resolve_entry_t *stub;
    resolve_entry_t *entry;
    u32_t *memloc;
    int i;
    for (i = 0; i < RESOLVE_ENTRIES; i++)
    {
        if (RESOLVE_TABLE[i].resolved)
        {
            continue;
        }
        stub = &RESOLVE_TABLE[i];
        // first attempt, look up in table
        if ((entry = uvl_resolve_table_get (stub->nid, 1)) != NULL)
        {
            stub->type = entry->type;
            switch (entry->type)
            {
                case RESOLVE_TYPE_FUNCTION:
                    /*
                    MOV  R12, (u16_t)entry->value.func_ptr
                    MOVT R12, (u16_t)(entry->value.func_ptr >> 16)
                    BX   R12
                    */
                    memloc = stub->value.func_ptr;
                    memloc[0] = uvl_encode_arm_inst (INSTRUCTION_MOV, (u16_t)entry->value.value, 12);
                    memloc[1] = uvl_encode_arm_inst (INSTRUCTION_MOVT, (u16_t)(entry->value.value >> 16), 12);
                    memloc[2] = uvl_encode_arm_inst (INSTRUCTION_BRANCH, 0, 12);
                    break;
                case RESOLVE_TYPE_SYSCALL:
                    /*
                    MOV  R12, (u16_t)entry->value.value
                    SVC  0
                    BX   LR
                    */
                    memloc = stub->value.func_ptr;
                    memloc[0] = uvl_encode_arm_inst (INSTRUCTION_MOV, (u16_t)entry->value.value, 12);
                    memloc[1] = uvl_encode_arm_inst (INSTRUCTION_SYSCALL, 0, 0);
                    memloc[2] = uvl_encode_arm_inst (INSTRUCTION_BRANCH, 0, 14);
                    break;
                case RESOLVE_TYPE_VARIABLE:
                    *(u32_t*)stub->value.ptr = entry->value.value;
                    break;
            }
            stub->resolved = 1;
            continue;
        }
        // second attempt, estimate syscall
        if ((entry = uvl_estimate_syscall (stub->nid)) != NULL)
        {
            /*
            MOV  R12, (u16_t)entry->value.value
            SVC  0
            BX   LR
            */
            memloc = stub->value.func_ptr;
            memloc[0] = uvl_encode_arm_inst (INSTRUCTION_MOV, (u16_t)entry->value.value, 12);
            memloc[1] = uvl_encode_arm_inst (INSTRUCTION_SYSCALL, 0, 0);
            memloc[2] = uvl_encode_arm_inst (INSTRUCTION_BRANCH, 0, 14);
            stub->resolved = 1;
            continue;
        }
        // we failed :(
        LOG ("Failed to resolve function NID: 0x%08X, continuing", stub->nid);
    }
}

/********************************************//**
 *  \brief Adds entries from loaded modules to 
 *  resolve table
 *  
 *  This functions locates all loaded modules 
 *  and attempts to read their import and/or 
 *  export table and add them to our resolve 
 *  table.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_resolve_all_loaded_modules (int type) ///< An OR combination of flags (see defined "Search flags for importing loaded modules") directing the search
{
    loaded_module_info_t m_mod_info;
    module_info_t *mod_info;
    void *result;
    u32_t segment_size;
    SceUID mod_list[MAX_LOADED_MODS];
    u32_t num_loaded = MAX_LOADED_MODS;
    int i;
    module_exports_t *exports;
    module_imports_t *imports;

    if (sceKernelGetModuleList (0xFF, mod_list, &num_loaded) < 0)
    {
        LOG ("Failed to get module list.");
        return -1;
    }
    for (i = 0; i < num_loaded; i++)
    {
        m_mod_info.size = sizeof (loaded_module_info_t); // should be 440
        if (sceKernelGetModuleInfo (mod_list[i], &m_mod_info) < 0)
        {
            LOG ("Error getting info for mod 0x%08X, continuing", mod_list[i]);
            continue;
        }
        mod_info = NULL;
        result = m_mod_info.segments[0].vaddr;
        segment_size = m_mod_info.segments[0].memsz;
        for (;;)
        {
            result = memstr (m_mod_info.module_name, strlen (m_mod_info.module_name), result, segment_size);
            if (result == NULL)
            {
                break; // not found
            }
            // try making this the one
            mod_info = (module_info_t*)((u32_t)result - 4);
            if (PTR_VALID (mod_info->ent_top) && PTR_VALID (mod_info->stub_top))
            {
                break; // we found it
            }
            else // that string just happened to appear
            {
                segment_size -= (u32_t)result - (u32_t)m_mod_info.segments[0].vaddr;
                continue;
            }
        }
        if (mod_info == NULL)
        {
            LOG ("Can't get module information for %s.", m_mod_info.module_name);
            return -1;
        }
        if (BIT_SET (type, RESOLVE_MOD_EXPS))
        {
            for (exports = (module_exports_t*)((u32_t)m_mod_info.segments[0].vaddr + mod_info->ent_top); 
                (u32_t)exports < ((u32_t)m_mod_info.segments[0].vaddr + mod_info->ent_end); exports++)
            {
                if (uvl_add_resolved_exports (exports) < 0)
                {
                    LOG ("Unable to resolve exports at %p. Continuing.", exports);
                    continue;
                }
            }
        }
        if (BIT_SET (type, RESOLVE_MOD_IMPS))
        {
            for (imports = (module_exports_t*)(m_mod_info.segments[0].vaddr + mod_info->stub_top); 
                (u32_t)imports < ((u32_t)m_mod_info.segments[0].vaddr + mod_info->stub_end); imports++)
            {
                if (uvl_add_resolved_imports (imports, BIT_SET (type, RESOLVE_IMPS_SVC_ONLY)) < 0)
                {
                    LOG ("Unable to resolve imports at %p. Continuing.", imports);
                    continue;
                }
            }
        }
    }
}
