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

int START_SECTION uvl_start ();
int uvl_entry ();
int uvl_exit (int status);

#endif
/// @}
