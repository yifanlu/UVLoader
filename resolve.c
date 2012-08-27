#include "resolve.h"
#include "string.h"

// 1110 0011 0000 0000 1100 001010000011
// 1110 0011 0000 0101 1100 011100101001 // 0x5729
// 1110 0011 0100 1000 1100 000100110110 // 0x8136
// 1110 0001 0010 111111111111 0001 1100 // BX R12

#define BIT_SET(i, b) (i & (0x1 << b) == 0)
resolve_entry_t *RESOLVE_TABLE = 0x85000000;
int RESOLVE_ENTRIES = 0;

int uvl_resolve_table_add (resolve_entry_t *entry) // TODO: remove?
{
    void *location = &RESOLVE_TABLE[RESOLVE_ENTRIES];
    memcpy (location, entry);
    RESOLVE_ENTRIES++;
    return;
}

resolve_entry_t *uvl_resolve_table_get (u32_t nid, int forced_resolved)
{
    int i;
    for (i = 0; i < RESOLVE_ENTRIES; i++)
    {
        if (RESOLVE_TABLE[i].nid == nid)
        {
            // entries could be placeholders for resolving later
            if (forced_resolved && RESOLVE_TABLE[i].type == RESOLVE_TYPE_STUB)
            {
                continue;
            }
            return &RESOLVE_TABLE[i];
        }
    }
    return NULL;
}

int uvl_export_stub_to_entry (void *func, u32_t nid, resolve_entry_t *entry)
{
    entry->nid = nid;
    entry->type = RESOLVE_TYPE_FUNCTION; // exports are always functions
    entry->value.function_ptr = func;
    return 0;
}

int uvl_import_stub_to_entry (void *func, u32_t nid, resolve_entry_t *entry)
{
    u32_t val = 0;
    u8_t inst_type = 0;
    int read = 0;
    entry->type = RESOLVE_TYPE_UNRESOLVED;
    for (;;)
    {
        val = uvl_decode_arm_inst (func, &inst_type);
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
        switch (inst_type)
        {
            case INSTRUCTION_MOV:
                entry->value.value = val;
                break;
            case INSTRUCTION_MOVT:
                entry->value.func_ptr |= val << 16; // load upper
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
        if (entry->type != RESOLVE_TYPE_UNRESOLVED)
        {
            // we resolved it, get out before something weird happens
            break;
        }
    }
    if (entry->type = RESOLVE_TYPE_UNRESOLVED || entry->value.value == 0) // we failed :(
    {
        LOG ("Failed to resolve import NID: 0x%08X", nid);
        return -1;
    }
    // put the finishing touches
    entry->nid = nid;
    return 0;
}

u32_t uvl_decode_arm_inst (u32_t cur_inst, u8_t *type)
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

u32_t uvl_encode_arm_inst (u8_t type, u16_t immed, u16_t reg)
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

int uvl_add_resolved_imports (module_imports_t *imp_table)
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
        if (uvl_resolve_table_add (&entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    // get variables
    for(i = 0; i < imp_table->num_vars; i++)
    {
        res_entry->nid = imp_table->var_nid_table[i];
        res_entry->type = RESOLVE_TYPE_VARIABLE;
        res_entry->value.value = *(u32_t*)imp_table->var_entry_table[i];
        if (uvl_resolve_table_add (&entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    // get TLS
    // TODO: Find out how this works
    for(i = 0; i < imp_table->num_tls_vars; i++)
    {
        res_entry->nid = imp_table->tls_nid_table[i];
        res_entry->type = RESOLVE_TYPE_VARIABLE;
        res_entry->value.value = *(u32_t*)imp_table->tls_entry_table[i];
        if (uvl_resolve_table_add (&entry) < 0)
        {
            LOG ("Error adding entry to table.");
            return -1;
        }
    }
    return 0;
}
