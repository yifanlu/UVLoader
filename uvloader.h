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
#define UVL_VER_MAJOR   0   ///< Major version
#define UVL_VER_MINOR   1   ///< Minor version
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
    int (*dbglog)(const char *);                   ///< Debug logging (optional)
    void *libkernel_anchor;                        ///< Any imported SceLibKernel function
} uvl_context_t;

int START_SECTION uvl_start ();
int uvl_entry ();
int uvl_exit (int status);

#endif
/// @}
