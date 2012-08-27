#ifndef UVL_ELF
#define UVL_ELF

#include "types.h"

// ELF data types
typedef void*   Elf32_Addr;
typedef u32_t   Elf32_Off;
typedef u32_t   Elf32_Sword;
typedef u32_t   Elf32_Word;
typedef u16_t   Elf32_Half;

// ELF constants
#define EI_NIDENT   16
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_DATA     5
#define EI_VERSION  6
#define EI_PAD      7
#define ET_EXEC     2
#define EM_ARM      40
#define EV_CURRENT  1
#define ELFMAG0     0x7F
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

#define ELFCLASS32  1
#define ELFDATA2LSB 1

// other constants
#define MAGIC_LEN   4
#define SCEMAG0     'S'
#define SCEMAG1     'C'
#define SCEMAG2     'E'
#define SCEMAG3     0
#define SCEHDR_LEN  0xA0
#define SEC_MODINFO ".sceModuleInfo.rodata"

// ELF structs
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

// other structs
typedef struct module_info // thanks roxfan
{
    u16_t   modattribute;  // ??
    u8_t    modversion[2]; // always 1,1?
    char    modname[27];   // 
    u8_t    type;          // 6 = user-mode prx?
    void    *gp_value;     // always 0 on ARM
    u32_t   ent_top;       // beginning of the export list (sceModuleExports array)
    u32_t   ent_end;       // end of same
    u32_t   stub_top;      // beginning of the import list (sceModuleStubInfo array)
    u32_t   stub_end;      // end of same
    u32_t   module_nid;    // ID of the PRX? seems to be unused
    u32_t   field_38;      // unused in samples
    u32_t   field_3C;      // I suspect these may contain TLS info
    u32_t   field_40;      //
    u32_t   mod_start;     // module start function; can be 0 or -1; also present in exports
    u32_t   mod_stop;      // module stop function
    u32_t   exidx_start;   // ARM EABI style exception tables
    u32_t   exidx_end;     //
    u32_t   extab_start;   //
    u32_t   extab_end;     //
} module_info_t;

typedef struct module_exports // thanks roxfan
{
    u16_t   size;           // size of this structure; 0x20 for Vita 1.x
    u8_t    lib_version[2]; //
    u16_t   attribute;      // ?
    u16_t   num_functions;  // number of exported functions
    u32_t   num_vars;       // number of exported variables
    u32_t   num_tls_vars    // number of exported TLS variables?
    u32_t   module_nid;     // NID of this specific export list; one PRX can export several names
    char    *lib_name;      // name of the export module
    u32_t   *nid_table;     // array of 32-bit NIDs for the exports, first functions then vars
    void    **entry_table;  // array of pointers to exported functions and then variables
} module_exports_t;

typedef struct module_imports // thanks roxfan
{
    u16_t   size;               // size of this structure; 0x34 for Vita 1.x
    u8_t    lib_version[2];     //
    u16_t   attribute;          //
    u16_t   num_functions;      // number of imported functions
    u16_t   num_vars;           // number of imported variables
    u16_t   num_tls_vars;       // number of imported TLS variables
    u32_t   reserved1;          // ?
    u32_t   module_nid;         // NID of the module to link to
    char    *lib_name;          // name of module
    u32_t   reserved2;          // ?
    u32_t   *func_nid_table;    // array of function NIDs (numFuncs)
    void    **func_entry_table; // parallel array of pointers to stubs; they're patched by the loader to jump to the final code
    u32_t   *var_nid_table;     // NIDs of the imported variables (numVars)
    void    **var_entry_table;  // array of pointers to "ref tables" for each variable
    u32_t   *tls_nid_table;     // NIDs of the imported TLS variables (numTlsVars)
    void    **tls_entry_table;  // array of pointers to ???
} module_imports_t;

typedef union module_ports
{
    u16_t           size;
    module_imports  imports;
    module_exports  exports;
}

#endif
