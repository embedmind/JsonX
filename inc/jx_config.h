/**************************************************************************/
/*                                                                        */
/*  @file jx_config.h                                                     */
/*  @brief JsonX Library Configuration Header                             */
/*                                                                        */
/*  This file is part of the JsonX Library.                               */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#ifndef JX_CONFIG_H_
#define JX_CONFIG_H_

/**************************************************************************/
/*                                                                        */
/*  Configuration Macros                                                  */
/*                                                                        */
/**************************************************************************/

/**
 * @def JX_DEBUG
 *
 * @brief Enables debug output.
 *
 * When defined, internal messages are printed using the platform's
 * debug interface (see jx_debug_print()).
 */
#define JX_DEBUG

/**
 * @def JX_USE_BAREMETAL
 * @def JX_USE_RTOS
 * @def JX_USE_CUSTOM_ALLOCATOR
 *
 * @brief System integration mode selection.
 *
 * Exactly one of these must be defined:
 * - JX_USE_BAREMETAL for static or heap-based baremetal systems
 * - JX_USE_RTOS for RTOS-managed memory
 * - JX_USE_CUSTOM_ALLOCATOR for user-provided hooks
 */
//#define JX_USE_BAREMETAL
#define JX_USE_RTOS
//#define JX_USE_CUSTOM_ALLOCATOR

/**
 * @def JX_USE_THREADX
 * @def JX_USE_FREERTOS
 *
 * @brief RTOS selection (required if @ref JX_USE_RTOS is defined).
 *
 * If no RTOS is explicitly defined, ThreadX is assumed by default.
 */
#define JX_USE_THREADX
//#define JX_USE_FREERTOS

/**
 * @def JX_USE_HEAP_BAREMETAL
 *
 * @brief Enables dynamic heap allocation (malloc/free) on baremetal systems.
 *
 * When defined together with `JX_USE_BAREMETAL`, the JsonX memory manager will
 * allocate temporary buffers using malloc and free instead of a static buffer.
 *
 * @note This option is only valid when @ref JX_USE_BAREMETAL is defined.
 */
//#define JX_USE_HEAP_BAREMETAL

/**************************************************************************/
/* Sanity Checks                                                          */
/**************************************************************************/

/* Error if more than one integration mode is selected */
#if (defined(JX_USE_RTOS) + defined(JX_USE_BAREMETAL) + defined(JX_USE_CUSTOM_ALLOCATOR)) != 1
#error "Exactly one of JX_USE_RTOS, JX_USE_BAREMETAL, or JX_USE_CUSTOM_ALLOCATOR must be defined."
#endif

/* Error if multiple RTOS types are selected */
#if defined(JX_USE_THREADX) && defined(JX_USE_FREERTOS)
#error "Invalid configuration: Cannot define both JX_USE_THREADX and JX_USE_FREERTOS."
#endif

/* Error if RTOS type is not selected */
#if defined(JX_USE_RTOS)
    #if !defined(JX_USE_THREADX) && !defined(JX_USE_FREERTOS)
        #define JX_USE_THREADX
    #endif
#endif

/* Error if heap allocation is enabled outside of baremetal mode */
#if defined(JX_USE_HEAP_BAREMETAL) && !defined(JX_USE_BAREMETAL)
#error "JX_USE_HEAP_BAREMETAL requires JX_USE_BAREMETAL to be defined."
#endif

#if defined(JX_USE_CUSTOM_ALLOCATOR) && \
    (defined(JX_USE_RTOS) || defined(JX_USE_BAREMETAL))
#error "JX_USE_CUSTOM_ALLOCATOR cannot be combined with RTOS or BAREMETAL modes."
#endif

/**************************************************************************/
/* JsonX Limits                                                           */
/**************************************************************************/

/**
 * @def JX_MAX_NESTING_LEVEL
 *
 * @brief Maximum nesting level allowed when parsing JSON.
 *
 * Currently reserved for future use and not enforced.
 */
#define JX_MAX_NESTING_LEVEL     3

/**
 * @def JX_PROPERTY_MAX_SIZE
 *
 * @brief Maximum size (in characters) of a JSON property name.
 */
#define JX_PROPERTY_MAX_SIZE     50

#endif /* JX_CONFIG_H_ */
