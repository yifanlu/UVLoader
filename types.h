/// 
/// \file types.h
/// \brief Types used in UVLoader
/// 
#ifndef UVL_TYPES
#define UVL_TYPES

#include <stdint.h>

/** \name Fixed width integers
 *  @{
 */
typedef uint8_t u8_t;       ///< Unsigned 8-bit type
typedef uint16_t u16_t;     ///< Unsigned 16-bit type
typedef uint32_t u32_t;     ///< Unsigned 32-bit type
typedef uint64_t u64_t;     ///< Unsigned 64-bit type
/** @}*/

/** \name SCE standard types
 *  @{
 */
typedef int SceUID;         ///< SceUID type
typedef u64_t SceOff;       ///< SceOff type
typedef int SceSSize;       ///< SceSSize type
/** @}*/

// TODO: Make sure void* is 4 bytes

#define NULL 0

#endif
