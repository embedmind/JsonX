/**************************************************************************/
/*                                                                        */
/*  @file jx_static_allocator.c                                           */
/*  @brief Static Memory Allocator for Baremetal Mode (JsonX)             */
/*                                                                        */
/*  Implements fixed-buffer memory pool used when no RTOS or malloc()     */
/*  is available. Controlled by jx_config.h flags.                        */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/


#include "jx_config.h"

#ifdef JX_USE_BAREMETAL

#include "jx_static_allocator.h"

#ifdef JX_USE_HEAP_BAREMETAL
#include <stdlib.h>
#endif

#ifndef JX_USE_HEAP_BAREMETAL
static jx_static_allocator_t *allocator = NULL;

/**
 * @brief Initialize the static memory allocator.
 *
 * This function must be called once with a properly sized buffer.
 * It places the allocator instance at the beginning of the buffer
 * and uses the remaining space as a pool.
 *
 * @param buffer Pointer to user-allocated memory.
 * @param size   Total size of the buffer in bytes.
 * @return Pointer to allocator instance or NULL if size is insufficient.
 */
jx_static_allocator_t *jx_static_allocator_init(void *buffer, size_t size)
{
    if (!buffer || size < ALIGN_4(sizeof(jx_static_allocator_t)))
        return NULL;

    allocator = (jx_static_allocator_t *)buffer;
    allocator->pool_start  = (uint8_t *)buffer + ALIGN_4(sizeof(jx_static_allocator_t));
    allocator->pool_size   = size - ALIGN_4(sizeof(jx_static_allocator_t));
    allocator->pool_offset = 0;
    return allocator;
}
#endif /* !JX_USE_HEAP_BAREMETAL */

/**
 * @brief Allocate memory from static or dynamic memory pool.
 *
 * If JX_USE_HEAP_BAREMETAL is defined, malloc() is used.
 * Otherwise, memory is carved from the internal static pool.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to memory block or NULL on failure.
 */
void *jx_static_malloc(size_t size)
{
#ifdef JX_USE_HEAP_BAREMETAL
    void *ptr = malloc(ALIGN_4(size));
#else
    if (!allocator || size == 0)
        return NULL;

    size = ALIGN_4(size);
    if (allocator->pool_offset + size > allocator->pool_size)
        return NULL;

    void *ptr = allocator->pool_start + allocator->pool_offset;
    allocator->pool_offset += size;
#endif
    return ptr;
}

/**
 * @brief Free memory block.
 *
 * This is a no-op for static mode, but required to satisfy APIs
 * that expect a deallocation hook (e.g., cJSON).
 *
 * @param ptr Pointer to memory block (may be NULL).
 */
void jx_static_free(void *ptr)
{
#ifdef JX_USE_HEAP_BAREMETAL
    free(ptr);
#endif
    (void)ptr;// noop in static mode
}

#ifndef JX_USE_HEAP_BAREMETAL
/**
 * @brief Reset static allocator state.
 *
 * This reclaims the entire static pool for reuse.
 */
void jx_static_reset(void)
{
    if (allocator)
        allocator->pool_offset = 0;
}
#endif /* !JX_USE_HEAP_BAREMETAL */

#endif /* JX_USE_BAREMETAL */
