#ifndef UVL_RESOLVE
#define UVL_RESOLVE

#include "types.h"

#define RESOLVE_TYPE_UNKNOWN    0
#define RESOLVE_TYPE_FUNCTION   1
#define RESOLVE_TYPE_SYSCALL    2
#define RESOLVE_TYPE_VARIABLE   3

#define INSTRUCTION_UNKNOWN     0
#define INSTRUCTION_MOV         1
#define INSTRUCTION_MOVT        2
#define INSTRUCTION_SYSCALL     3
#define INSTRUCTION_BRANCH      4
#define STUB_FUNC_MAX_LEN       16

#define RESOLVE_MOD_IMPS        0x1
#define RESOLVE_MOD_EXPS        0x2

#define MAX_LOADED_MODS         128

typedef struct resolve_entry
{
    u32_t   nid;
    u16_t   type;
    u16_t   resolved;
    union value
    {
        u32_t   value;
        u32_t   ptr;
        u32_t   func_ptr;
        u32_t   syscall;
    };
} resolve_entry_t;

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
} module_ports_t;

typedef struct segment_info
{
    u32_t           size;  // this structure size (0x18)
    u32_t           perms; // probably rwx in low bits
    u32_t           vaddr; // address in memory
    u32_t           memsz; // size in memory
    u32_t           flags; // meanig unknown
    u32_t           res;   // unused?
} segment_info_t;

typedef struct loaded_module_info
{
    u32_t           size;               // 0x1B8 for Vita 1.x
    u32_t           handle;             // kernel module handle?
    u32_t           flags;              // some bits. could be priority or whatnot
    char            module_name[28];
    u32_t           unkn_28;
    void            *module_start;
    u32_t           unkn_30;
    void            *module_stop;
    void            *exidx_start;
    void            *exidx_end;
    u32_t           unkn_40;
    u32_t           unkn_40;
    void            *tls_init_data;
    u32_t           tls_init_size;
    u32_t           tls_area_size;
    char            file_path[256];         // 
    segment_info_t  segments[4];
    u32_t           type;       // 6 = user-mode PRX?
} loaded_module_info_t;

#endif
