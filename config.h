/// 
/// \file config.h
/// \brief Constants specific to current exploit
/// @{
/// 
#ifndef UVL_CONFIG
#define UVL_CONFIG

#define HOMEBREW_PATH               "cache0:/VitaInjectorClient/Temp/homebrew.self"     ///< Where to load the homebrew.
#define LOG_PATH                    "cache0:/VitaInjectorClient/Temp/uvloader.log"     ///< Where to load the homebrew.
#define PRINT_DEBUG_LOCATION        (void**) 0x817C3C80                 ///< Location of a function pointer for console debug output.
//#define RESOLVE_TABLE_LOCATION      (void*) 0x817C3C84                  ///< Location of a writable area of memory to store resolve table.

#endif
/// @}
