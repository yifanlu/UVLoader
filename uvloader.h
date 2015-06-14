/// 
/// \file uvloader.h
/// \brief Userland Vita Loader startup
/// \defgroup uvloader UVLoader
/// \brief Startup and hooks
/// @{
/// 
#ifndef UVL_MAIN
#define UVL_MAIN

#define START_SECTION __attribute__ ((section (".text.start")))
#define EXIT_NID        0x826BBBAF      ///< NID of C exit() call

/** \name UVLoader version information
 *  @{
 */
#define UVL_VER_MAJOR   1   ///< Major version
#define UVL_VER_MINOR   0   ///< Minor version
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
} uvl_context_t;

int START_SECTION uvl_start (uvl_context_t *ctx);
int uvl_entry ();
int uvl_exit (int status);

void *uvl_alloc_code_mem (unsigned int *p_len);
void uvl_unlock_mem ();
void uvl_lock_mem ();
void uvl_flush_icache (void *addr, unsigned int len);
int uvl_debug_log (const char *line);

#endif
/// @}
