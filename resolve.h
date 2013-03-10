/// 
/// \file resolve.h
/// \brief Functions to resolve NIDs and syscalls
/// \defgroup resolve Executable Resolver
/// \brief Finds and resolves NIDs
/// @{
/// 
#ifndef UVL_RESOLVE
#define UVL_RESOLVE

#include "types.h"

/** \name Type of entry
 *  @{
 */
#define RESOLVE_TYPE_UNKNOWN    0       ///< Unknown type
#define RESOLVE_TYPE_FUNCTION   1       ///< Function call
#define RESOLVE_TYPE_SYSCALL    2       ///< Syscall
#define RESOLVE_TYPE_VARIABLE   3       ///< Imported variable
/** @}*/

/** \name Supported ARM instruction types
 *  @{
 */
#define INSTRUCTION_UNKNOWN     0       ///< Unknown/unsupported instruction
#define INSTRUCTION_MOVW        1       ///< MOVW Rd, \#imm instruction
#define INSTRUCTION_MOVT        2       ///< MOVT Rd, \#imm instruction
#define INSTRUCTION_SYSCALL     3       ///< SVC \#imm instruction
#define INSTRUCTION_BRANCH      4       ///< BX Rn instruction
/** @}*/

#define STUB_FUNC_MAX_LEN       16      ///< Max size for a stub function in bytes

/** \name Search flags for importing loaded modules
 *  \sa uvl_resolve_all_loaded_modules
 *  @{
 */
#define RESOLVE_MOD_IMPS        0x1     ///< Add entry from import function stubs
#define RESOLVE_MOD_EXPS        0x2     ///< Add entry from exported information
#define RESOLVE_IMPS_SVC_ONLY   0x4     ///< Used with @c RESOLVE_MOD_IMPS but only add syscalls for import entries
/** @}*/

/** \name Module infomation check
 *  @{
 */
#define MOD_INFO_VALID_ATTR     0x0000
#define MOD_INFO_VALID_VER      0x0101
/** @}*/

#define MAX_LOADED_MODS         128     ///< Maximum number of loaded modules
#define MAX_RESOLVE_ENTRIES     0x10000 ///< Maximum number of resolves
#define STUB_FUNC_SIZE          0x10    ///< Size of stub functions
#define UVL_LIBKERN_BASE        0xE0010000   ///< sceLibKernel is where we import API calls from
#define UVL_LIBKERN_MAX_SIZE    0xE000  ///< Maximum size of sceLibKernel (for resolving loader)

/**
 * \brief Resolve table entry
 * 
 * This can represent either an unresolved entry 
 * from a homebrew or a resolved entry copied 
 * from a loaded module.
 */
typedef struct resolve_entry
{
    u32_t   nid;            ///< NID of entry
    u16_t   type;           ///< See defined "Type of entry"
    u16_t   reserved;       ///< For future use
    /**
     * \brief Value of the entry
     */
    union value
    {
        void*   ptr;        ///< A pointer value for unresolved pointing to where to write resolved value
        u32_t   value;      ///< Any double-word value for resolved
        void*   func_ptr;   ///< A pointer to stub function for unresolved or function to call for resolved
        u32_t   syscall;    ///< A syscall number
    } value;
} resolve_entry_t;

/**
 * \brief SCE module information section
 * 
 * Can be found in an ELF file or loaded in 
 * memory.
 */
typedef struct module_info // thanks roxfan
{
    u16_t   modattribute;  // ??
    u16_t   modversion;    // always 1,1?
    char    modname[27];   ///< Name of the module
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

/**
 * \brief SCE module export table
 * 
 * Can be found in an ELF file or loaded in 
 * memory.
 */
typedef struct module_exports // thanks roxfan
{
    u16_t   size;           // size of this structure; 0x20 for Vita 1.x
    u8_t    lib_version[2]; //
    u16_t   attribute;      // ?
    u16_t   num_functions;  // number of exported functions
    u32_t   num_vars;       // number of exported variables
    u32_t   num_tls_vars;   // number of exported TLS variables?  <-- pretty sure wrong // yifanlu
    u32_t   module_nid;     // NID of this specific export list; one PRX can export several names
    char    *lib_name;      // name of the export module
    u32_t   *nid_table;     // array of 32-bit NIDs for the exports, first functions then vars
    void    **entry_table;  // array of pointers to exported functions and then variables
} module_exports_t;

/**
 * \brief SCE module import table
 * 
 * Can be found in an ELF file or loaded in 
 * memory.
 */
typedef struct module_imports // thanks roxfan
{
    u16_t   size;               // size of this structure; 0x34 for Vita 1.x
    u16_t   lib_version;        //
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

/**
 * \brief Either an SCE module import table or export table
 * 
 * \sa module_imports
 * \sa module_exports
 */
typedef union module_ports
{
    u16_t             
    size;       ///< Size of table
    module_imports_t  imports;    ///< Import kind
    module_exports_t  exports;    ///< Export kind
} module_ports_t;

/**
 * \brief A segment of the module in memory
 */
typedef struct segment_info
{
    u32_t           size;   // this structure size (0x18)
    u32_t           perms;  // probably rwx in low bits
    void            *vaddr; // address in memory
    u32_t           memsz;  // size in memory
    u32_t           flags;  // meanig unknown
    u32_t           res;    // unused?
} segment_info_t;

/**
 * \brief Loaded module information
 * 
 * Returned by @c sceKernelGetModuleInfo
 */
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
    u32_t           unkn_44;
    void            *tls_init_data;
    u32_t           tls_init_size;
    u32_t           tls_area_size;
    char            file_path[256];         // 
    segment_info_t  segments[4];
    u32_t           type;       // 6 = user-mode PRX?
} loaded_module_info_t;

/** \name Interacting with the resolve table
 *  @{
 */
int uvl_resolve_table_initialize ();
int uvl_resolve_table_destroy ();
int uvl_resolve_table_add (resolve_entry_t *entry);
resolve_entry_t *uvl_resolve_table_get (u32_t nid);
/** @}*/
/** \name Estimating syscalls
 *  @{
 */
resolve_entry_t *uvl_estimate_syscall (u32_t nid);
/** @}*/
/** \name Capturing and resolving stubs
 *  @{
 */
int uvl_resolve_import_stub_to_entry (void *stub, u32_t nid, resolve_entry_t *entry);
int uvl_resolve_entry_to_import_stub (resolve_entry_t *entry, void *stub);
/** @}*/
/** \name ARM instruction functions
 *  @{
 */
u32_t uvl_decode_arm_inst (u32_t cur_inst, u8_t *type);
u32_t uvl_encode_arm_inst (u8_t type, u16_t immed, u16_t reg);
/** @}*/
/** \name Bulk add to resolve table
 *  @{
 */
int uvl_resolve_add_imports (module_imports_t *imp_table, int syscalls_only);
int uvl_resolve_add_exports (module_exports_t *exp_table);
/** @}*/
/** \name Resolving entries
 *  @{
 */
int uvl_resolve_add_all_modules (int type);
int uvl_resolve_add_module (PsvUID modid, int type);
int uvl_resolve_imports (module_imports_t *import);
int uvl_resolve_loader (u32_t nid, void *libkernel_base, void *stub);
/** @}*/

// live resolving too slow
#if 0
/** \name Resolving entries
 *  @{
 */
u32_t uvl_encode_arm_inst (u8_t type, u16_t immed, u16_t reg);
int uvl_resolve_stub (u32_t nid, void *stub, char *lib_name);
int uvl_resolve_stub_from_module (u32_t nid, void *stub, char *lib_name, void *mod_start, module_info_t *mod_info);
/** @}*/
#endif

#endif
/// @}
