/**************************************************************************/
/*                                                                        */
/*  @file jx_config.h                                                     */
/*  @brief JsonX compile-time configuration boundary                      */
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
/*  User Configuration Override                                           */
/*                                                                        */
/**************************************************************************/

/*
 * Projects may provide jx_user_config.h on the compiler include path before
 * the JsonX inc/ directory. That file owns project/profile-specific choices
 * such as RTOS integration, debug logging, and optional parser features.
 */
#if defined(__has_include)
#if __has_include("jx_user_config.h")
#include "jx_user_config.h"
#endif
#endif

/**************************************************************************/
/*                                                                        */
/*  Integration Defaults                                                  */
/*                                                                        */
/**************************************************************************/

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
 *
 * If a project override does not choose an integration mode, JsonX defaults
 * to bare-metal static allocation.
 */
#if !defined(JX_USE_BAREMETAL) && \
    !defined(JX_USE_RTOS) && \
    !defined(JX_USE_CUSTOM_ALLOCATOR)
#define JX_USE_BAREMETAL
#endif

/**
 * @def JX_USE_THREADX
 * @def JX_USE_FREERTOS
 *
 * @brief RTOS selection required when @ref JX_USE_RTOS is defined.
 *
 * If RTOS mode is selected and no RTOS is explicitly provided, ThreadX is
 * assumed for backwards compatibility.
 */
#if defined(JX_USE_RTOS) && !defined(JX_USE_THREADX) && !defined(JX_USE_FREERTOS)
#define JX_USE_THREADX
#endif

/**
 * @def JX_ENABLE_DOUBLE
 *
 * @brief Enables legacy double-backed `JX_NUMBER` mappings.
 *
 * Keep disabled unless fractional JSON values are required. Firmware config
 * should prefer typed integer mappings.
 */
#ifndef JX_ENABLE_DOUBLE
#define JX_ENABLE_DOUBLE 0
#endif

/**
 * @def JX_ENABLE_JSON_COMMENTS
 *
 * @brief Enables JSONC-style comments in the native parser.
 *
 * When set to `1`, whitespace skipping also accepts line comments and
 * C-style block comments outside strings. Serialization always emits strict
 * JSON without comments.
 */
#ifndef JX_ENABLE_JSON_COMMENTS
#define JX_ENABLE_JSON_COMMENTS 0
#endif

/**
 * @def JX_MAX_NESTING_LEVEL
 *
 * @brief Maximum nesting level allowed when parsing JSON.
 */
#ifndef JX_MAX_NESTING_LEVEL
#define JX_MAX_NESTING_LEVEL     3
#endif

/**
 * @def JX_PROPERTY_MAX_SIZE
 *
 * @brief Maximum size in characters for JSON property names and legacy
 * fallback string buffers. Prefer explicit string capacity in mappings.
 */
#ifndef JX_PROPERTY_MAX_SIZE
#define JX_PROPERTY_MAX_SIZE     50
#endif

/**************************************************************************/
/*                                                                        */
/*  Sanity Checks                                                         */
/*                                                                        */
/**************************************************************************/

#if (defined(JX_USE_RTOS) + defined(JX_USE_BAREMETAL) + defined(JX_USE_CUSTOM_ALLOCATOR)) != 1
#error "Exactly one of JX_USE_RTOS, JX_USE_BAREMETAL, or JX_USE_CUSTOM_ALLOCATOR must be defined."
#endif

#if defined(JX_USE_THREADX) && defined(JX_USE_FREERTOS)
#error "Invalid configuration: Cannot define both JX_USE_THREADX and JX_USE_FREERTOS."
#endif

#if defined(JX_USE_HEAP_BAREMETAL) && !defined(JX_USE_BAREMETAL)
#error "JX_USE_HEAP_BAREMETAL requires JX_USE_BAREMETAL to be defined."
#endif

#if defined(JX_USE_CUSTOM_ALLOCATOR) && \
    (defined(JX_USE_RTOS) || defined(JX_USE_BAREMETAL))
#error "JX_USE_CUSTOM_ALLOCATOR cannot be combined with RTOS or BAREMETAL modes."
#endif

#endif /* JX_CONFIG_H_ */
