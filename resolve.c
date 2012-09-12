#include "config.h"
#include "resolve.h"
#include "scefuncs.h"
#include "utils.h"

/** Checks if a bit is set in a given member */
#define BIT_SET(i, b) (i & (0x1 << b))

/** Stores resolve entries */
struct resolve_table {
    PsvUID             block_uid;   ///< UID of the memory block for freeing
    u32_t              length;      ///< Number of entries
    resolve_entry_t    table[];     ///< Table entries
} *g_resolve_table = NULL;

/********************************************//**
 *  \brief Allocates memory for resolve table.
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_resolve_table_initialize ()
{
    u32_t size;
    PsvUID block;
    void *base;

    size = (MAX_RESOLVE_ENTRIES * sizeof (resolve_entry_t) + sizeof (u32_t) + sizeof (PsvUID) + 0xFFF) & ~0xFFF; // store block id, counter, table, and align to 0x1000 bytes
    IF_DEBUG LOG ("Creating resolve table of size %u.", size);
    block = sceKernelAllocMemBlock ("UVLTable", 0xC20D060, size, NULL);
    if (block < 0)
    {
        LOG ("Error allocating resolve table. 0x%08X", block);
        return -1;
    }
    if (sceKernelGetMemBlockBase (block, &base) < 0)
    {
        LOG ("Error getting block base.");
        return -1;
    }
    IF_DEBUG LOG ("Block UID 0x%08X allocated at 0x%08X", (u32_t)block, (u32_t)base);
    psvUnlockMem ();
    g_resolve_table = base;
    psvLockMem ();
    g_resolve_table->block_uid = block;
    g_resolve_table->length = 0;
    return 0;
}

/********************************************//**
 *  \brief Frees memory for resolve table.
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_resolve_table_destroy ()
{
    PsvUID block;

    if (g_resolve_table == NULL)
    {
        IF_DEBUG LOG ("Resolve table not initialized.");
        return 0;
    }
    if (sceKernelFreeMemBlock (g_resolve_table->block_uid) < 0)
    {
        LOG ("Error freeing resolve table.");
        return -1;
    }
    psvUnlockMem ();
    g_resolve_table = NULL;
    psvLockMem ();
    return 0;
}

/********************************************//**
 *  \brief Adds a resolve entry
 *  
 *  This function does not check for duplicates.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_resolve_table_add (resolve_entry_t *entry) ///< Entry to add
{
    if (g_resolve_table->length >= MAX_RESOLVE_ENTRIES)
    {
        LOG ("Resolve table full. Please recompile with larger table.");
        return -1;
    }
    IF_VERBOSE LOG ("Adding entry #%u to resolve table.", g_resolve_table->length);
    IF_VERBOSE LOG ("NID: 0x%08X, type: %u, value 0x%08X", entry->nid, entry->type, entry->value.value);
    memcpy (&g_resolve_table->table[g_resolve_table->length], entry, sizeof (resolve_entry_t));
    g_resolve_table->length++;
    return 0;
}

/********************************************//**
 *  \brief Gets a resolve entry
 *  
 *  This function returns the first entry in the 
 *  table that has the given NID.
 *  \returns Entry on success, NULL on error
 ***********************************************/
resolve_entry_t *
uvl_resolve_table_get (u32_t nid)               ///< NID to resolve
{
    int i;
    for (i = 0; i < g_resolve_table->length; i++)
    {
        if (g_resolve_table->table[i].nid == nid)
        {
            return &g_resolve_table->table[i];
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
uvl_resolve_import_stub_to_entry (void *stub,  ///< Stub function to read
                                 u32_t nid,    ///< NID resolved by stub
                       resolve_entry_t *entry) ///< Entry to write to
{
    u32_t val = 0;
    u8_t inst_type = 0;
    int read = 0;
    entry->type = RESOLVE_TYPE_UNKNOWN;
    entry->value.value = 0;
    for (;;)
    {
        val = uvl_decode_arm_inst (*(u32_t*)stub, &inst_type);
        switch (inst_type)
        {
            case INSTRUCTION_MOVW:
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
                IF_DEBUG LOG ("Invalid instruction: 0x%08X found at 0x%08X for NID 0x%08X", *(u32_t*)stub, (u32_t)stub, nid);
                break;
        }
        if (entry->type != RESOLVE_TYPE_UNKNOWN)
        {
            break;
        }
        // next instruction
        stub = (char*)stub + sizeof (u32_t);
    }
    if (entry->value.value == 0) // false alarm
    {
        LOG ("Stub 0x%08X has not been resolved yet. Skipping.", (u32_t)stub);
        return -1;
    }
    // put the finishing touches
    entry->nid = nid;
    IF_VERBOSE LOG ("Mapped resolved import NID: 0x%X to 0x%X of type %u.", entry->nid, entry->value.value, entry->type);
    return 0;
}

/********************************************//**
 *  \brief Fills a stub with resolve entry
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_resolve_entry_to_import_stub (resolve_entry_t *entry,   ///< Entry to read from
                                             void *stub)    ///< Stub function to fill
{
    u32_t *memloc = stub;
    switch (entry->type)
    {
        case RESOLVE_TYPE_FUNCTION:
            memloc[0] = uvl_encode_arm_inst (INSTRUCTION_MOVW, (u16_t)entry->value.value, 12);
            memloc[1] = uvl_encode_arm_inst (INSTRUCTION_MOVT, (u16_t)(entry->value.value >> 16), 12);
            memloc[2] = uvl_encode_arm_inst (INSTRUCTION_BRANCH, 0, 12);
            break;
        case RESOLVE_TYPE_SYSCALL:
            memloc[0] = uvl_encode_arm_inst (INSTRUCTION_MOVW, (u16_t)entry->value.value, 12);
            memloc[1] = uvl_encode_arm_inst (INSTRUCTION_SYSCALL, 0, 0);
            memloc[2] = uvl_encode_arm_inst (INSTRUCTION_BRANCH, 0, 14);
            break;
        case RESOLVE_TYPE_VARIABLE:
            memloc[0] = entry->value.value;
            break;
        case RESOLVE_TYPE_UNKNOWN:
        default:
            LOG ("Invalid resolve entry 0x%08X", (u32_t)entry);
            return -1;
    }
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
        if (BIT_SET (cur_inst, 26) && BIT_SET (cur_inst, 25) && BIT_SET (cur_inst, 24))
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
            *type = INSTRUCTION_MOVW;
        }
        // Immediate value at 19-16 and 11-0
        // discard bytes 31-20
        // discard bytes 15-0
        return (((cur_inst << 12) >> 28) << 12) | ((cur_inst << 20) >> 20);
    }
    // Bits 27-25 should be 000 for jump instructions
    else if (!BIT_SET (cur_inst, 26) && !BIT_SET (cur_inst, 25))
    {
        // Bits 24-4 should be 100101111111111110001, 0x12FFF1 for BX
        if ((cur_inst << 7) >> 11 == 0x12FFF1)
        {
            *type = INSTRUCTION_BRANCH;
            return 1;
        }
        // Bits 24-4 should be 100101111111111110001, 0x12FFF3 for BLX
        else if ((cur_inst << 7) >> 11 == 0x12FFF3)
        {
            *type = INSTRUCTION_BRANCH;
            return 1;
        }
        else
        {
            // unknown jump
            return -1;
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
        case INSTRUCTION_MOVW:
            // 1110 0011 0000 XXXX YYYY XXXXXXXXXXXX
            // where X is the immediate and Y is the register
            // Upper bits == 0xE30
            return ((u32_t)0xE30 << 20) | ((u32_t)(immed & 0xF000) << 4) | (immed & 0xFFF) | (reg << 12);
        case INSTRUCTION_MOVT:
            // 1110 0011 0100 XXXX YYYY XXXXXXXXXXXX
            // where X is the immediate and Y is the register
            // Upper bits == 0xE34
            return ((u32_t)0xE34 << 20) | ((u32_t)(immed & 0xF000) << 4) | (immed & 0xFFF) | (reg << 12);
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
uvl_resolve_add_imports (module_imports_t *imp_table,    ///< Module's import table to read from
                                       int syscalls_only) ///< If set, will only add resolved syscalls and nothing else
{
    // this should be called BEFORE cleanup
    resolve_entry_t res_entry;
    u32_t *memory;
    u32_t nid;
    int i;
    // get functions first
    IF_VERBOSE LOG ("Found %u resolved function imports to copy.", imp_table->num_functions);
    for(i = 0; i < imp_table->num_functions; i++)
    {
        nid = imp_table->func_nid_table[i];
        memory = imp_table->func_entry_table[i];
        if (uvl_resolve_import_stub_to_entry (memory, nid, &res_entry) < 0)
        {
            LOG ("Error generating entry from import stub. Continuing.");
            continue;
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
    IF_VERBOSE LOG ("Found %u resolved variable imports to copy.", imp_table->num_vars);
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
    IF_VERBOSE LOG ("Found %u resolved tls imports to copy.", imp_table->num_tls_vars);
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

#if 0
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
    IF_DEBUG LOG ("Found %u unresolved function imports to copy.", imp_table->num_functions);
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
    IF_DEBUG LOG ("Found %u unresolved variable imports to copy.", imp_table->num_vars);
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
    IF_DEBUG LOG ("Found %u unresolved tls imports to copy.", imp_table->num_tls_vars);
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
#endif

/********************************************//**
 *  \brief Add a module's export entries to 
 *  the resolve table
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_resolve_add_exports (module_exports_t *exp_table) ///< Module's export table
{
    resolve_entry_t res_entry;
    int i;
    int offset = 0;

    // get functions first
    res_entry.type = RESOLVE_TYPE_FUNCTION;
    IF_VERBOSE LOG ("Found %u resolved function exports to copy.", exp_table->num_functions);
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
    IF_VERBOSE LOG ("Found %u resolved variable exports to copy.", exp_table->num_vars);
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
    return 0;
}

#if 0
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
    IF_DEBUG LOG ("%u resolve entries to look through.", g_resolve_entries);
    for (i = 0; i < g_resolve_entries; i++)
    {
        if (g_resolve_table[i].resolved)
        {
            continue;
        }
        stub = &g_resolve_table[i];
        // first attempt, look up in table
        IF_DEBUG LOG ("Trying to lookup NID 0x%X in resolve table.", stub->nid);
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
        IF_DEBUG LOG ("Trying to estimate syscall for NID 0x%X.", stub->nid);
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
#endif

/********************************************//**
 *  \brief Adds entries from all loaded 
 *  modules to resolve table
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int 
uvl_resolve_add_all_modules (int type) ///< An OR combination of flags (see defined "Search flags for importing loaded modules") directing the search
{
    PsvUID mod_list[MAX_LOADED_MODS];
    u32_t num_loaded = MAX_LOADED_MODS;
    int i;

    IF_DEBUG LOG ("Getting list of loaded modules.");
    if (sceKernelGetModuleList (0xFF, mod_list, &num_loaded) < 0)
    {
        LOG ("Failed to get module list.");
        return -1;
    }
    IF_DEBUG LOG ("Found %u loaded modules.", num_loaded);
    for (i = 0; i < num_loaded; i++)
    {
        if (uvl_resolve_add_module (mod_list[i], type) < 0)
        {
            LOG ("Failed to add module %u: 0x%08X. Continuing.", i, mod_list[i]);
            continue;
        }
    }
    return 0;
}

/********************************************//**
 *  \brief Adds entries from a loaded module to 
 *  resolve table
 *  
 *  This functions takes a loaded module 
 *  and attempts to read its import and/or 
 *  export table and add entries to our 
 *  resolve table.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_resolve_add_module (PsvUID modid, ///< UID of the module
                           int type)  ///< An OR combination of flags (see defined "Search flags for importing loaded modules") directing the search
{
    loaded_module_info_t m_mod_info;
    module_info_t *mod_info;
    void *result;
    u32_t segment_size;
    module_exports_t *exports;
    module_imports_t *imports;

    m_mod_info.size = sizeof (loaded_module_info_t); // should be 440
    IF_VERBOSE LOG ("Getting information for module UID: 0x%X.", modid);
    if (sceKernelGetModuleInfo (modid, &m_mod_info) < 0)
    {
        LOG ("Error getting info for mod 0x%08X", modid);
        return -1;
    }
    IF_VERBOSE LOG ("Module: %s, file: %s", m_mod_info.module_name, m_mod_info.file_path);
    mod_info = NULL;
    result = m_mod_info.segments[0].vaddr;
    segment_size = m_mod_info.segments[0].memsz;
    while (segment_size > 0)
    {
        IF_VERBOSE LOG ("Searching for module name in memory. Start 0x%X", result);
        result = memstr (result, segment_size, m_mod_info.module_name, strlen (m_mod_info.module_name));
        if (result == NULL)
        {
            IF_DEBUG LOG ("Cannot find module name in memory.");
            break; // not found
        }
        // try making this the one
        mod_info = (module_info_t*)((u32_t)result - 4);
        IF_VERBOSE LOG ("Possible module info struct at 0x%X", (u32_t)mod_info);
        if (mod_info->modattribute == MOD_INFO_VALID_ATTR && mod_info->modversion == MOD_INFO_VALID_VER) // TODO: Better check
        {
            IF_VERBOSE LOG ("Module export start at 0x%X import start at 0x%X", (u32_t)mod_info->ent_top + (u32_t)m_mod_info.segments[0].vaddr, (u32_t)mod_info->stub_top + (u32_t)m_mod_info.segments[0].vaddr);
            break; // we found it
        }
        else // that string just happened to appear
        {
            IF_DEBUG LOG ("False alarm, found name is not in module info structure.");
            mod_info = NULL;
            segment_size -= ((u32_t)result - (u32_t)m_mod_info.segments[0].vaddr) + strlen (m_mod_info.module_name); // subtract length
            result = (void*)((u32_t)result + strlen (m_mod_info.module_name)); // start after name
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
        IF_VERBOSE LOG ("Adding exports to resolve table.");
        for (exports = (module_exports_t*)((u32_t)m_mod_info.segments[0].vaddr + mod_info->ent_top); 
            (u32_t)exports < ((u32_t)m_mod_info.segments[0].vaddr + mod_info->ent_end); exports++)
        {
            if (exports->lib_name != NULL)
            {
                IF_VERBOSE LOG ("Adding exports for %s", exports->lib_name);
            }
            if (uvl_resolve_add_exports (exports) < 0)
            {
                LOG ("Unable to resolve exports at 0x%08X. Continuing.", (u32_t)exports);
                continue;
            }
        }
    }
    if (BIT_SET (type, RESOLVE_MOD_IMPS))
    {
        IF_VERBOSE LOG ("Adding resolved imports to resolve table.");
        for (imports = (module_imports_t*)((u32_t)m_mod_info.segments[0].vaddr + mod_info->stub_top); 
            (u32_t)imports < ((u32_t)m_mod_info.segments[0].vaddr + mod_info->stub_end); imports++)
        {
            IF_VERBOSE LOG ("Adding imports for %s", imports->lib_name);
            if (uvl_resolve_add_imports (imports, BIT_SET (type, RESOLVE_IMPS_SVC_ONLY)) < 0)
            {
                LOG ("Unable to resolve imports at 0x%08X. Continuing.", (u32_t)imports);
                continue;
            }
        }
    }
    return 0;
}

/********************************************//**
 *  \brief Resolves an import table
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_resolve_imports (module_imports_0945_t *import)   ///< Import table
{
    u32_t i;
    resolve_entry_t *resolve;
    u32_t *stub;

    IF_DEBUG LOG ("Resolving import table at 0x%08X", (u32_t)import);
    for (i = 0; i < import->num_functions; i++)
    {
        IF_VERBOSE LOG ("Trying to resolve function NID: 0x%08X found in %s", import->func_nid_table[i], import->lib_name);
        resolve = uvl_resolve_table_get (import->func_nid_table[i]);
        stub = import->func_entry_table[i];
        IF_VERBOSE LOG ("Stub located at: 0x%08X", (u32_t)stub);
        if (resolve == NULL)
        {
            LOG ("Cannot resolve NID: 0x%08X. Continuing.", import->func_nid_table[i]);
            continue;
        }
        if (uvl_resolve_entry_to_import_stub (resolve, stub) < 0)
        {
            LOG ("Cannot write to stub 0x%08X", (u32_t)stub);
            return -1;
        }

    }
    for (i = 0; i < import->num_vars; i++)
    {
        IF_VERBOSE LOG ("Trying to resolve variable NID: 0x%08X found in %s", import->var_nid_table[i], import->lib_name);
        resolve = uvl_resolve_table_get (import->var_nid_table[i]);
        stub = import->var_entry_table[i];
        IF_VERBOSE LOG ("Stub located at: 0x%08X", (u32_t)stub);
        if (resolve == NULL)
        {
            LOG ("Cannot resolve NID: 0x%08X. Continuing.", import->var_nid_table[i]);
            continue;
        }
        if (uvl_resolve_entry_to_import_stub (resolve, stub) < 0)
        {
            LOG ("Cannot write to stub 0x%08X", (u32_t)stub);
            return -1;
        }
    }
    for (i = 0; i < import->num_tls_vars; i++)
    {
        IF_VERBOSE LOG ("Trying to resolve tls NID: 0x%08X found in %s", import->tls_nid_table[i], import->lib_name);
        resolve = uvl_resolve_table_get (import->tls_nid_table[i]);
        stub = import->tls_entry_table[i];
        IF_VERBOSE LOG ("Stub located at: 0x%08X", (u32_t)stub);
        if (resolve == NULL)
        {
            LOG ("Cannot resolve NID: 0x%08X. Continuing.", import->tls_nid_table[i]);
            continue;
        }
        if (uvl_resolve_entry_to_import_stub (resolve, stub) < 0)
        {
            LOG ("Cannot write to stub 0x%08X", (u32_t)stub);
            return -1;
        }
    }
    return 0;
}

// live resolving is too slow
#if 0
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
        case INSTRUCTION_MOVW:
            // 1110 0011 0000 XXXX YYYY XXXXXXXXXXXX
            // where X is the immediate and Y is the register
            // Upper bits == 0xE30
            return ((u32_t)0xE30 << 20) | ((u32_t)(immed & 0xF000) << 4) | (immed & 0xFFF) | (reg << 12);
        case INSTRUCTION_MOVT:
            // 1110 0011 0100 XXXX YYYY XXXXXXXXXXXX
            // where X is the immediate and Y is the register
            // Upper bits == 0xE34
            return ((u32_t)0xE34 << 20) | ((u32_t)(immed & 0xF000) << 4) | (immed & 0xFFF) | (reg << 12);
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
 *  \brief Resolves a NID using all loaded 
 *  modules
 *  
 *  Searches every loaded module in memory for 
 *  the NID.
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_resolve_stub (u32_t nid,       ///< NID to search for
                   void *stub,     ///< The stub to fill
                   char *lib_name) ///< A hint to which library to look in, can be NULL
{
    loaded_module_info_t m_mod_info;
    module_info_t *mod_info;
    void *result;
    u32_t segment_size;
    PsvUID mod_list[MAX_LOADED_MODS];
    u32_t num_loaded = MAX_LOADED_MODS;
    int i;

    IF_DEBUG LOG ("Looking for 0x%08X, which resides in %s, stub at 0x%08X", nid, lib_name, (u32_t)stub);
    IF_DEBUG LOG ("Getting list of loaded modules.");
    if (sceKernelGetModuleList (0xFF, mod_list, &num_loaded) < 0)
    {
        LOG ("Failed to get module list.");
        return -1;
    }
    IF_DEBUG LOG ("Found %u loaded modules.", num_loaded);
    for (i = 0; i < num_loaded; i++)
    {
        m_mod_info.size = sizeof (loaded_module_info_t); // should be 440
        IF_DEBUG LOG ("Getting information for module #%u, UID: 0x%X.", i, mod_list[i]);
        if (sceKernelGetModuleInfo (mod_list[i], &m_mod_info) < 0)
        {
            LOG ("Error getting info for mod 0x%08X, continuing", mod_list[i]);
            continue;
        }
        IF_DEBUG LOG ("Module: %s, file: %s", m_mod_info.module_name, m_mod_info.file_path);
        mod_info = NULL;
        result = m_mod_info.segments[0].vaddr;
        segment_size = m_mod_info.segments[0].memsz;
        while (segment_size > 0)
        {
            IF_DEBUG LOG ("Searching for module name in memory. Start 0x%X", result);
            result = memstr (result, segment_size, m_mod_info.module_name, strlen (m_mod_info.module_name));
            if (result == NULL)
            {
                IF_DEBUG LOG ("Cannot find module name in memory.");
                break; // not found
            }
            // try making this the one
            mod_info = (module_info_t*)((u32_t)result - 4);
            IF_DEBUG LOG ("Possible module info struct at 0x%X", (u32_t)mod_info);
            if (mod_info->modattribute == MOD_INFO_VALID_ATTR && mod_info->modversion == MOD_INFO_VALID_VER) // TODO: Better check
            {
                IF_DEBUG LOG ("Module export start at 0x%X import start at 0x%X", (u32_t)mod_info->ent_top + (u32_t)m_mod_info.segments[0].vaddr, (u32_t)mod_info->stub_top + (u32_t)m_mod_info.segments[0].vaddr);
                break; // we found it
            }
            else // that string just happened to appear
            {
                IF_DEBUG LOG ("False alarm, found name is not in module info structure.");
                mod_info = NULL;
                segment_size -= ((u32_t)result - (u32_t)m_mod_info.segments[0].vaddr) + strlen (m_mod_info.module_name); // subtract length
                result = (void*)((u32_t)result + strlen (m_mod_info.module_name)); // start after name
                continue;
            }
        }
        if (mod_info == NULL)
        {
            LOG ("Can't get module information for %s. Continuing.", m_mod_info.module_name);
            continue;
        }
        if (uvl_resolve_stub_from_module (nid, stub, lib_name, m_mod_info.segments[0].vaddr, mod_info) < 0)
        {
            IF_DEBUG LOG ("Failed to resolve NID with module %s. Continuing.", m_mod_info.module_name);
            continue;
        }
        else
        {
            IF_DEBUG LOG ("Resolved NID with module %s.", m_mod_info.module_name);
            return 0;
        }
    }
}

/********************************************//**
 *  \brief Resolves a NID exports/imports of 
 *  a loaded module.
 *  
 *  \returns Zero if found, otherwise not found
 ***********************************************/
int
uvl_resolve_stub_from_module (u32_t nid,        ///< NID to search for
                               void *stub,      ///< The stub to fill
                               char *lib_name,  ///< A hint to which library to look in, can be NULL
                               void *mod_start, ///< Start of the module to check in
                      module_info_t *mod_info)  ///< Information of the module to check in
{
    module_exports_t *exports;
    module_imports_t *imports;
    u32_t *memloc = stub;
    u32_t i;

    IF_DEBUG LOG ("Looking for 0x%08X, which resides in %s, stub at 0x%08X in %s (0x%08X)", nid, lib_name, (u32_t)stub, mod_info->modname, (u32_t)mod_start);
    for (exports = (module_exports_t*)((u32_t)mod_start + mod_info->ent_top); 
        (u32_t)exports < ((u32_t)mod_start + mod_info->ent_end); exports++)
    {
        if (!(lib_name > 0 && exports->lib_name > 0 && strcmp (lib_name, exports->lib_name) == 0))
        {
            // skip this one, not the right name
            continue;
        }
        for (i = 0; i < exports->num_functions + exports->num_vars; i++)
        {
            if (exports->nid_table[i] == nid)
            {
                // found it
                psvUnlockMem ();
                if (i >= exports->num_functions) // variable
                {
                    IF_DEBUG LOG ("Resolving NID: 0x%08X as variable valued: 0x%08X", nid, *(u32_t*)exports->entry_table[i]);
                    *memloc = *(u32_t*)exports->entry_table[i];
                }
                else // function
                {
                    IF_DEBUG LOG ("Resolving NID: 0x%08X as function pointer: 0x%08X", nid, (u32_t)exports->entry_table[i]);
                    memloc[0] = uvl_encode_arm_inst (INSTRUCTION_MOVW, (u16_t)(u32_t)exports->entry_table[i], 12);
                    memloc[1] = uvl_encode_arm_inst (INSTRUCTION_MOVT, (u16_t)((u32_t)exports->entry_table[i] >> 16), 12);
                    memloc[2] = uvl_encode_arm_inst (INSTRUCTION_BRANCH, 0, 12);
                }
                psvLockMem ();
                return 0; // resolved
            }
        }
    }
    for (imports = (module_imports_t*)((u32_t)mod_start + mod_info->stub_top); 
        (u32_t)imports < ((u32_t)mod_start + mod_info->stub_end); imports++)
    {
        if (!(lib_name > 0 && imports->lib_name > 0 && strcmp (lib_name, imports->lib_name) == 0))
        {
            // skip this one, not the right name
            continue;
        }
        for (i = 0; i < imports->num_functions; i++)
        {
            if (imports->func_nid_table[i] == nid)
            {
                // found it
                psvUnlockMem ();
                IF_DEBUG LOG ("Resolving NID: 0x%08X as copy of resolved stub at: 0x%08X", nid, (u32_t)imports->func_entry_table[i]);
                if (memcpy (stub, imports->func_entry_table[i], STUB_FUNC_SIZE) < 0)
                {
                    LOG ("Failed to copy resolved stub function.");
                    continue;
                }
                psvLockMem ();
                return 0; // resolved
            }
        }
        for (i = 0; i < imports->num_vars; i++)
        {
            if (imports->var_nid_table[i] == nid)
            {
                // found it
                psvUnlockMem ();
                IF_DEBUG LOG ("Resolving NID: 0x%08X as variable valued: 0x%08X", nid, *(u32_t*)imports->var_entry_table[i]);
                *memloc = *(u32_t*)imports->var_entry_table[i];
                psvLockMem ();
                return 0; // resolved
            }
        }
        for (i = 0; i < imports->num_tls_vars; i++)
        {
            if (imports->tls_nid_table[i] == nid)
            {
                // found it
                psvUnlockMem ();
                IF_DEBUG LOG ("Resolving NID: 0x%08X as tls variable valued: 0x%08X", nid, *(u32_t*)imports->tls_entry_table[i]);
                *memloc = *(u32_t*)imports->tls_entry_table[i];
                psvLockMem ();
                return 0; // resolved
            }
        }
    }
    return -1; // can't resolve it
}
#endif
