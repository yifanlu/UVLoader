/*
 * relocate.c - Performs SCE ELF relocations
 * Copyright 2015 Yifan Lu
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
#include "load.h"
#include "relocate.h"

/********************************************//**
 *  \brief Perform SCE relocation
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
static int
uvl_relocate_write (Elf32_Phdr_t *seg,  ///< Where to write
                    u32_t offset,       ///< Offset to write to
                    void *value,        ///< What to write
                    u32 size)           ///< How much
{
    return 0;
}

/********************************************//**
 *  \brief Perform SCE relocation
 *  
 *  \returns Zero on success, otherwise error
 ***********************************************/
int
uvl_relocate (void *reloc,          ///< Base address of relocation segment
              u32_t size,           ///< Size of relocation segment
              Elf32_Phdr_t *segs)   ///< All segments loaded
{
    sce_reloc_t *entry;
    u32_t pos;
    u16_t r_code;
    u32_t r_offset;
    u32_t r_addend;
    u8_t r_symseg;
    u8_t r_datseg;
    u32_t S, A, P;
    u32_t value;

    pos = 0;
    while (pos < size)
    {
        // get entry
        entry = (sce_reloc_t *)((char *)reloc + pos);
        if (SCE_RELOC_IS_SHORT (&entry))
        {
            r_offset = SCE_RELOC_SHORT_OFFSET (entry->r_short);
            r_addend = SCE_RELOC_SHORT_ADDEND (entry->r_short);
            pos += 8;
        }
        else
        {
            r_offset = SCE_RELOC_LONG_OFFSET (entry->r_long);
            r_addend = SCE_RELOC_LONG_ADDEND (entry->r_long);
            if (SCE_RELOC_LONG_CODE2 (entry->r_long))
            {
                IF_VERBOSE LOG ("Code2 ignored for relocation at %X.", pos);
            }
            pos += 12;
        }

        // get values
        r_symseg = SCE_RELOC_SYMSEG (&entry);
        r_datseg = SCE_RELOC_DATSEG (&entry);
        S = r_symseg == 15 ? 0 : segs[r_symseg].p_vaddr;
        P = segs[r_datseg].p_vaddr + r_offset;
        A = r_addend;

        // perform relocation
        switch (SCE_RELOC_CODE (&entry))
        {
            case R_ARM_NONE:
            case R_ARM_V4BX:
                break;
            case R_ARM_ABS32:
            case R_ARM_TARGET1:
                {
                    value = S + A;
                    uvl_relocate_write (&segs[r_datseg], r_offset, &value, sizeof (u32_t));
                }
                break;
            case R_ARM_REL32:
            case R_ARM_TARGET2:
                {
                    value = S + A - P;
                }
                break;
            case R_ARM_THM_CALL:
                {

                }
                break;
            case R_ARM_CALL:
            case R_ARM_JUMP24:
                {

                }
                break;
            case R_ARM_PREL31:
                {

                }
                break;
            case R_ARM_MOVW_ABS_NC:
                {

                }
                break;
            case R_ARM_MOVT_ABS:
                {

                }
                break;
            case R_ARM_THM_MOVW_ABS_NC:
                {

                }
                break;
            case R_ARM_THM_MOVT_ABS:
                {

                }
                break;
            default
                {
                    LOG ("Unknown relocation code %d\n", r_code);
                }
                break;
        }
    }
}
