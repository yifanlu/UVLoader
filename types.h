/// 
/// \file types.h
/// \brief Types used in UVLoader
/// 
#ifndef UVL_TYPES
#define UVL_TYPES

/** \name Fixed width integers
 *  @{
 */
typedef unsigned char u8_t;             ///< Unsigned 8-bit type
typedef unsigned short int u16_t;       ///< Unsigned 16-bit type
typedef unsigned int u32_t;             ///< Unsigned 32-bit type
typedef unsigned long long int u64_t;        ///< Unsigned 64-bit type
typedef char s8_t;             ///< Signed 8-bit type
typedef short int s16_t;       ///< Signed 16-bit type
typedef int s32_t;             ///< Signed 32-bit type
typedef long long int s64_t;        ///< Signed 64-bit type
/** @}*/

/** \name SCE standard types
 *  @{
 */
typedef int PsvUID;            ///< PsvUID type
typedef long int PsvOff;       ///< PsvOff type
typedef int PsvSSize;          ///< PsvSSize type
/** @}*/

// TODO: Make sure void* is 4 bytes

#define NULL 0              ///< For NULL pointers

#endif
