/**************************************************************************/
/*                                                                        */
/*  @file jx_static_allocator.h                                           */
/*  @brief Interface for Static Buffer Allocator (JsonX)                  */
/*                                                                        */
/*  Provides memory allocation routines for baremetal deployments         */
/*  without dynamic heap or RTOS support.                                 */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/


#ifndef JX_STATIC_ALLOCATOR_H
#define JX_STATIC_ALLOCATOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "jx_config.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/*                                                                        */
/*  HOW TO USE                                                            */
/*                                                                        */
/*  1. Allocate a buffer large enough to hold the allocator structure     */
/*     and the memory pool.                                               */
/*                                                                        */
/*     Example:                                                           */
/*     uint8_t buffer[sizeof(jx_static_allocator_t) + POOL_SIZE];         */
/*                                                                        */
/*  2. Call jx_static_allocator_init(buffer, sizeof(buffer)) once.       */
/*                                                                        */
/*  3. Use jx_static_malloc() to allocate memory from the static pool.    */
/*     Call jx_static_reset() to reuse the entire pool.                   */
/*                                                                        */
/**************************************************************************/

#define ALIGN_4(x)  (((x) + 3) & ~3)

/**
 * @brief Static allocator context structure.
 */
typedef struct jx_static_allocator_t
{
    uint8_t  *pool_start;     ///< Start of memory pool (after instance)
    size_t    pool_size;      ///< Total size of memory pool
    size_t    pool_offset;    ///< Current allocation offset in pool
} jx_static_allocator_t;

/**
 * @brief Initialize the static allocator using the given buffer.
 *
 * The allocator structure is placed at the beginning of the buffer.
 *
 * @param buffer Pointer to a user-allocated memory buffer.
 * @param size   Total size of the buffer (including allocator and pool).
 * @return Pointer to allocator instance or NULL if size is too small.
 */
jx_static_allocator_t *jx_static_allocator_init(void *buffer, size_t size);

/**
 * @brief Allocate memory from the static memory pool.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory or NULL if not enough space.
 */
void *jx_static_malloc(size_t size);

/**
 * @brief Free a block of memory (no-op for static allocator).
 *
 * @param ptr Pointer to previously allocated block.
 */
void jx_static_free(void *ptr);

#ifndef JX_USE_HEAP_BAREMETAL
/**
 * @brief Reset the allocator to reuse the entire pool.
 */
void jx_static_reset(void);
#else
// Heap mode: no reset needed
#endif

#ifdef __cplusplus
}
#endif

#endif /* JX_STATIC_ALLOCATOR_H */
