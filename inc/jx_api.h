/**************************************************************************/
/*                                                                        */
/*  @file jx_api.h                                                        */
/*  @brief Public API header for JsonX Library                            */
/*                                                                        */
/*  This file is part of the JsonX Library.                               */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#ifndef JX_API_H
#define JX_API_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/*                                                                        */
/*  Include Files                                                         */
/*                                                                        */
/**************************************************************************/

#include "jx_version.h"
#include "jx_types.h"
#include "jx_user.h"

#ifdef JX_DEBUG
#include "jx_debug.h"
#else
#define IF_JX_ERROR_EXIT(object) if ((object) == NULL) { goto end; }
#endif

/**************************************************************************/
/*                                                                        */
/*  API Function Prototypes                                               */
/*                                                                        */
/**************************************************************************/


/**
 * @brief Initialize the JsonX library memory allocator.
 *
 * This function must be called before using any JsonX API that relies on
 * internal memory allocation (e.g., structure-to-JSON or JSON-to-structure).
 *
 * Behavior depends on the system integration mode:
 *
 * - **ThreadX integration**: the function takes a pointer to a `TX_BYTE_POOL`
 *   which is used for all dynamic allocations within the library.
 *
 * - **Baremetal mode**: the function expects a pointer to a user-provided
 *   memory buffer and its size. This buffer will be used as a static memory pool.
 *
 * @note This function must be called once during system startup.
 *       Calling it multiple times without calling @ref jx_deinit is not supported.
 *
 * @param[in] byte_pool Pointer to ThreadX byte pool (if @ref JX_USE_THREADX is defined).
 * @param[in] buffer    Pointer to the static buffer (baremetal only).
 * @param[in] size      Size of the static buffer in bytes (baremetal only).
 *
 * @retval JX_SUCCESS   Initialization successful.
 * @retval JX_ERROR     Initialization failed (e.g., invalid parameters or internal error).
 */
#ifdef JX_USE_THREADX
JX_STATUS jx_init(TX_BYTE_POOL *byte_pool);
#endif

#if  defined(JX_USE_FREERTOS) || defined(JX_USE_HEAP_BAREMETAL)
JX_STATUS jx_init(void);
#endif

#ifdef JX_USE_BAREMETAL
#ifndef JX_USE_HEAP_BAREMETAL
JX_STATUS jx_init(void *buffer, size_t size);
#endif
#endif
#ifdef JX_USE_CUSTOM_ALLOCATOR
JX_STATUS jx_init(JX_HOOKS *hooks);
#endif


/**
 * @brief Deinitialize the JsonX parser and release internal memory.
 *
 * This function releases all resources associated with the internal memory pool.
 * It should be called during system shutdown or before reinitialization.
 *
 * On RTOS-based systems, this function will release memory back to the
 * RTOS allocator (e.g., ThreadX byte pool). On baremetal systems, it resets
 * the internal static allocator.
 *
 * @note This function is safe to call even if the parser was not fully initialized.
 */
void jx_parser_deinit(void);

#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
/**
 * @brief Allocate memory from the internal memory pool used by JsonX.
 *
 * This function provides dynamic memory allocation compatible with the
 * selected system configuration:
 *
 * - **RTOS systems**: memory is allocated from the configured RTOS allocator (e.g., ThreadX pool).
 * - **Baremetal systems**: memory is carved from a statically provided buffer.
 *
 * This function is used internally by JsonX, but can also be used by the user
 * for allocating temporary buffers for operations like serialization or parsing.
 *
 * @param memory_size  Number of bytes to allocate.
 *
 * @return Pointer to the allocated memory, or NULL if allocation failed.
 *
 * @note Do not use standard malloc/free together with this function.
 */
void *jx_alloc_memory(size_t memory_size);

/**
 * @brief Free memory allocated with @ref jx_alloc_memory.
 *
 * This function returns memory to the internal allocator.
 * On RTOS systems, memory is released back to the RTOS pool.
 * On baremetal systems, this function is a no-op (placeholder),
 * as static memory cannot be freed selectively.
 *
 * @param memory_ptr Pointer to memory previously returned by @ref jx_alloc_memory.
 *
 * @note Calling this function with NULL is safe (no effect).
 */
void jx_free_memory(void *memory_ptr);
#endif

/**
 * @brief Convert a structure represented by JX_ELEMENTs into a JSON string.
 *
 * This function transforms a flat structure array into a JSON string and writes
 * the result into the user-provided buffer.
 *
 * The output buffer must be allocated by the user and passed as @p buffer.
 * If the JsonX library is used with RTOS integration (e.g., ThreadX), it is
 * possible to allocate this buffer using @ref jx_alloc_memory. In that case,
 * the user is responsible for calling @ref jx_free_memory to avoid memory leaks.
 *
 * On baremetal systems, static memory is used internally, and no dynamic allocation occurs.
 *
 * When RTOS is enabled, all temporary memory (e.g., for intermediate cJSON objects)
 * that is allocated from the internal memory pool is automatically freed at the
 * end of the operation.
 *
 * @note This is a high-level API function. Internal helpers are not exposed to
 *       avoid misuse or unsafe memory handling.
 *
 * @param element        Pointer to an array of JX_ELEMENTs describing the structure.
 * @param element_size   Number of elements in the @p element array.
 * @param buffer         Destination buffer to write the resulting JSON string.
 * @param buffer_size    Size of the destination buffer in bytes.
 * @param format         Output format (JX_FORMATTED or JX_MINIFIED).
 *
 * @retval JX_SUCCESS    The conversion was successful.
 * @retval JX_ERROR      Failed to serialize the structure or buffer is too small.
 */JX_STATUS jx_struct_to_json(JX_ELEMENT *element,
                            size_t element_size,
                            char *buffer,
                            size_t buffer_size,
                            JX_FORMAT format);

/**
 * @brief Parse a JSON string into a flat structure represented by JX_ELEMENTs.
 *
 * This function parses the given JSON string and populates the user-defined
 * structure described by the JX_ELEMENT array.
 *
 * Memory management is handled internally. On baremetal systems, the internal
 * memory buffer is reset before each call.
 *
 * When RTOS is enabled, all temporary memory (e.g., for intermediate cJSON objects)
 * that is allocated from the internal memory pool is automatically freed at the
 * end of the operation.
 *
 * @note This is a high-level API function. Internal functions used during
 *       parsing are private and not intended to be called directly.
 *
 * @param buffer         Pointer to the input JSON string.
 * @param element        Pointer to the array of JX_ELEMENTs representing the structure.
 * @param element_size   Number of elements in the @p element array.
 * @param mode           Parsing mode (e.g., JX_STRICT or JX_FLEXIBLE).
 *
 * @retval JX_SUCCESS    The structure was successfully parsed and filled.
 * @retval JX_ERROR      Invalid JSON, parsing failure, or unsupported format.
 */
JX_STATUS jx_json_to_struct(char *buffer,
                            JX_ELEMENT *element,
                            size_t element_size,
                            JX_PARSE_MODE mode);


#ifdef __cplusplus
}
#endif

#endif /* JX_API_H */
