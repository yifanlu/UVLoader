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

/** \name UVLoader version information
 *  @{
 */
#define UVL_VER_MAJOR   0   ///< Major version
#define UVL_VER_MINOR   0   ///< Minor version
#define UVL_VER_REV     1   ///< Revision
/** @}*/

int START_SECTION _start ();
int uvl_entry ();

#endif
/// @}
