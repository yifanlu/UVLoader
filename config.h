/// 
/// \file config.h
/// \brief Constants specific to current exploit
/// @{
/// 
#ifndef UVL_CONFIG
#define UVL_CONFIG

#define HOMEBREW_PATH               "cache0:/UVLoader/homebrew.elf"     ///< Where to load the homebrew.
#define PRINT_DEBUG_LOCATION        (void**) 0x85000000                 ///< Location of a function pointer for console debug output.
#define RESOLVE_TABLE_LOCATION      (void**) 0x85000004                 ///< Location of a writable area of memory to store resolve table.

#endif
/// @}
