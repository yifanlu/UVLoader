/// 
/// \file cleanup.h
/// \brief Functions to clean up memory
/// \defgroup cleanup Memory Cleanup
/// \brief Frees memory before loading
/// @{
/// 
#ifndef UVL_CLEANUP
#define UVL_CLEANUP

#include "types.h"

int uvl_cleanup_memory ();
int uvl_unload_all_modules ();

#endif
/// @}
