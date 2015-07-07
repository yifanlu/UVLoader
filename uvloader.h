/// 
/// \file uvloader.h
/// \brief Userland Vita Loader startup
/// \defgroup uvloader UVLoader
/// \brief Startup and hooks
/// @{
/// 
#ifndef UVL_MAIN
#define UVL_MAIN

#include "types.h"

#define START_SECTION __attribute__ ((section (".text.start")))
#define NAKED __attribute__((naked))
#define UVL_EXIT_NID        	0x826BBBAF      ///< NID of C exit() call
#define UVL_PRINTF_NID        	0x9A004680      ///< NID of C printf() call
#define UVL_CODE_ALLOC_NID      0xBCEAB831      ///< uvl_alloc_code_mem
#define UVL_CODE_UNLOCK_NID     0x98D1C91D      ///< uvl_unlock_mem
#define UVL_CODE_LOCK_NID       0xEEC99826      ///< uvl_lock_mem
#define UVL_CODE_FLUSH_NID      0xC85B400C      ///< uvl_flush_icache
#define UVL_DEBUG_LOG_NID       0xD4F59028      ///< uvl_debug_log
#define UVL_SET_HOOK_NID        0x57FC7C6A      ///< uvl_set_hook
#define UVL_LOAD_NID            0xE8E92954      ///< uvl_load

#define LOADED_INFO_SIZE        0x1000          ///< Must be page aligned

/** \name UVLoader version information
 *  @{
 */
#define UVL_VER_MAJOR   1   ///< Major version
#define UVL_VER_MINOR   0   ///< Minor version
#define UVL_VER_REV     1   ///< Revision
/** @}*/

/**
 * \brief UVLoader bootstrap information
 * 
 * External data passed to start of UVLoader 
 * to aid in locating functions and bypassing 
 * ASLR.
 */
typedef struct uvl_context
{
    void *(*psvCodeAllocMem)(unsigned int *p_len); ///< Allocate code block
    void (*psvUnlockMem)(void);                    ///< Unlock code block
    void (*psvLockMem)(void);                      ///< Relock code block
    void (*psvFlushIcache)(void *, unsigned int);  ///< Flush Icache
    int (*logline)(const char *);                  ///< Debug logging (optional)
    void *libkernel_anchor;                        ///< Any imported SceLibKernel function
} uvl_context_t;

/**
 * \brief Callback exit function
 */
typedef void (*uvl_exit_cb_t)(int status);

/**
 * \brief Loaded application information
 * 
 * Information on the loaded application.
 */
typedef struct uvl_loaded
{
    struct uvl_loaded *next;    ///< Stack of loaded applications
    PsvUID self_uid;            ///< UID for this block
    void *saved_sp;             ///< Stack pointer on application entry
    uvl_exit_cb_t callback;     ///< Callback on exit
    int numsegs;                ///< Number of elements in array below
    PsvUID segs[0];             ///< Array of memory segments allocated
} uvl_loaded_t;

int START_SECTION uvl_start (uvl_context_t *ctx);
int uvl_start_load ();
void uvl_final (int status);

/** \name UVLoader API Exports
 *  @{
 */
void *uvl_alloc_code_mem (unsigned int *p_len);
void uvl_unlock_mem ();
void uvl_lock_mem ();
void uvl_flush_icache (void *addr, unsigned int len);
int uvl_debug_log (const char *line);
int uvl_load (const char *path);
int uvl_set_hook (uvl_exit_cb_t callback);
void uvl_exit (int status);
int printf (const char *format, ...);
/** @}*/

#endif
/// @}
