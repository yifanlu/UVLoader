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
#define UVL_EXIT_NID            0x826BBBAF      ///< NID of C exit() call
#define UVL_PRINTF_NID          0x9A004680      ///< NID of C printf() call
#define UVL_CODE_ALLOC_NID      0xBCEAB831      ///< uvl_alloc_code_mem
#define UVL_CODE_UNLOCK_NID     0x98D1C91D      ///< uvl_unlock_mem
#define UVL_CODE_LOCK_NID       0xEEC99826      ///< uvl_lock_mem
#define UVL_CODE_FLUSH_NID      0xC85B400C      ///< uvl_flush_icache
#define UVL_DEBUG_LOG_NID       0xD4F59028      ///< uvl_debug_log
#define UVL_LOAD_NID            0xE8E92954      ///< uvl_load

#define UVL_LOG_WRITE_NID       0x10000001      ///< uvl_log_write

#define LOADED_INFO_SIZE        0x1000          ///< Must be page aligned

/** \name UVLoader version information
 *  @{
 */
#define UVL_VER_MAJOR   1   ///< Major version
#define UVL_VER_MINOR   2   ///< Minor version
#define UVL_VER_REV     0   ///< Revision
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
    int use_debugnet;                              ///< Use debugnet logging
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
    PsvUID tid;                 ///< UID of running thread
    int stack_size;             ///< Stack size of loaded application
    int thread_priority;        ///< Priority of loaded application
    int numsegs;                ///< Number of elements in array below
    PsvUID segs[0];             ///< Array of memory segments allocated
} uvl_loaded_t;

int START_SECTION uvl_start (uvl_context_t *ctx);
int uvl_start_load ();

/** \name UVLoader API Exports
 *  @{
 */
void *uvl_alloc_code_mem (unsigned int *p_len);
void uvl_unlock_mem ();
void uvl_lock_mem ();
void uvl_flush_icache (void *addr, unsigned int len);
int uvl_debug_log_psm (const char *line);
int uvl_debug_log (const char *line);
int uvl_load (const char *path);
void uvl_exit (int status);

int uvl_log_write(const void* buffer, u32_t size);

typedef int(*debug_log_func)(const char* line);
void uvl_set_debug_log_func(debug_log_func func);
/** @}*/

#endif
/// @}
