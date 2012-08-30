/// 
/// \file load.h
/// \brief Functions to load executable
/// \defgroup load Executable Loader
/// \brief Parses and loads ELFs
/// @{
/// 
#ifndef UVL_LOAD
#define UVL_LOAD

#include "types.h"

/** \name ELF data types
 *  @{
 */
typedef void*   Elf32_Addr;
typedef u32_t   Elf32_Off;
typedef u32_t   Elf32_Sword;
typedef u32_t   Elf32_Word;
typedef u16_t   Elf32_Half;
/** @}*/

/** \name ELF identification
 *  @{
 */
#define EI_NIDENT   16
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_PAD      7
#define ELFMAG0     0x7F
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'
#define ELFCLASS32  1
#define ELFDATA2LSB 1
/** @}*/
/** \name ELF object types
 *  @{
 */
#define ET_EXEC     2
/** @}*/
/** \name ELF machine types
 *  @{
 */
#define EM_ARM      40
/** @}*/
/** \name ELF version
 *  @{
 */
#define EV_CURRENT  1
/** @}*/

/** \name SCE identification
 *  @{
 */
#define MAGIC_LEN   4
#define SCEMAG0     'S'
#define SCEMAG1     'C'
#define SCEMAG2     'E'
#define SCEMAG3     0
#define SCEHDR_LEN  0xA0
/** @}*/
#define SEC_MODINFO ".sceModuleInfo.rodata" ///< Name of module information section

/** \name ELF structures
 *  See the ELF specification for more information.
 *  @{
 */
typedef struct Elf32_Ehdr
{
    u8_t        e_ident[EI_NIDENT];
    Elf32_Half  e_type;
    Elf32_Half  e_machine;
    Elf32_Word  e_version;
    Elf32_Addr  e_entry;
    Elf32_Off   e_phoff;
    Elf32_Off   e_shoff;
    Elf32_Word  e_flags;
    Elf32_Half  e_ehsize;
    Elf32_Half  e_phentsize;
    Elf32_Half  e_phnum;
    Elf32_Half  e_shentsize;
    Elf32_Half  e_shnum;
    Elf32_Half  e_shstrndx;
} Elf32_Ehdr_t;

typedef struct Elf32_Shdr
{
    Elf32_Word  sh_name;
    Elf32_Word  sh_type;
    Elf32_Word  sh_flags;
    Elf32_Addr  sh_addr;
    Elf32_Off   sh_offset;
    Elf32_Word  sh_size;
    Elf32_Word  sh_link;
    Elf32_Word  sh_info;
    Elf32_Word  sh_addralign;
    Elf32_Word  sh_entsize;
} Elf32_Shdr_t;

typedef struct Elf32_Phdr
{
    Elf32_Word  p_type;
    Elf32_Off   p_offset;
    Elf32_Addr  p_vaddr;
    Elf32_Addr  p_paddr;
    Elf32_Word  p_filesz;
    Elf32_Word  p_memsz;
    Elf32_Word  p_flags;
    Elf32_Word  p_align;
} Elf32_Phdr_t;
/** @}*/

/** \name Functions to load code
 *  @{
 */
int uvl_load_exe (const char *filename, void **entry);
int uvl_load_elf (SceUID fd, SceOff start_offset, void **entry);
int uvl_load_module (char *name);
/** @}*/
/** \name Helper functions
 *  @{
 */
int uvl_check_elf_header (Elf32_Ehdr_t *hdr);
int uvl_get_module_info (SceUID fd, SceOff start_offset, Elf32_Ehdr_t *elf_hdr, module_info_t *mod_info);
/** @}*/

#endif
/// @}
