/**************************************************************************/
/*                                                                        */
/*  @file jx_internal.h                                                   */
/*  @brief Internal Macros and Assertions for JsonX                       */
/*                                                                        */
/*  This file is part of the JsonX Library.                               */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/


#ifndef JX_INTERNAL_H
#define JX_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/*                                                                        */
/*  Include Files                                                         */
/*                                                                        */
/**************************************************************************/

#include "jx_types.h"
#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
#include "jx_static_allocator.h"
#endif
#include <string.h>

#ifdef JX_DEBUG
#include "jx_debug.h"
#else
#define IF_JX_ERROR_EXIT(object) if ((object) == NULL) { goto end; }
#endif

/**************************************************************************/
/*                                                                        */
/*  Internal Macros                                                       */
/*                                                                        */
/**************************************************************************/

/* Return JX_ERROR if pointer is NULL. */
#define JX_RETURN_IF_NULL(p)        \
    if ((p) == NULL)                \
    {                               \
        return JX_ERROR;            \
    }

/**************************************************************************/
/*                                                                        */
/*  Inline Helpers                                                        */
/*                                                                        */
/**************************************************************************/

/* JSON parser instance */
typedef struct
{
    JX_STATE   state;
    JX_STATUS  status;
    JX_HOOKS   hooks;

#ifdef JX_USE_THREADX
    TX_BYTE_POOL *byte_pool;
#endif
#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
    jx_static_allocator_t* allocator;
#endif
} JX_PARSER;

/**
 * @brief Check if the element has a non-empty property name.
 *
 * @param e Pointer to JX_ELEMENT.
 * @return true if property is not empty, false otherwise.
 */
static inline bool jx_has_property(const JX_ELEMENT *e)
{
    return (e && e->property[0] != '\0');
}

/**
 * @brief Set the element status to UPDATED.
 *
 * @param e Pointer to JX_ELEMENT.
 */
static inline void jx_set_updated(JX_ELEMENT *e)
{
    e->status = JX_ELEMENT_UPDATED;
}

/**
 * @brief Clear the element update status.
 *
 * @param e Pointer to JX_ELEMENT.
 */
static inline void jx_clear_status(JX_ELEMENT *e)
{
    e->status = JX_ELEMENT_NOT_UPDATED;
}

/**
 * @brief Check if the element is marked as updated.
 *
 * @param e Pointer to JX_ELEMENT.
 * @return true if updated, false otherwise.
 */
static inline bool jx_is_updated(JX_ELEMENT *e)
{
    return e->status == JX_ELEMENT_UPDATED;
}

/**
 * @brief Set a boolean value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Boolean value to set.
 */
static inline void jx_set_bool(JX_ELEMENT *e, bool value)
{
    *((bool *)(uintptr_t)(e->value_p)) = value;
    jx_set_updated(e);
}

/**
 * @brief Set a number (double) value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Double value to set.
 */
static inline void jx_set_number(JX_ELEMENT *e, double value)
{
    *((double *)(uintptr_t)(e->value_p)) = value;
    jx_set_updated(e);
}

/**
 * @brief Set an unsigned 32-bit integer value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Unsigned 32-bit integer value to set.
 */
static inline void jx_set_u32(JX_ELEMENT *e, uint32_t value)
{
    *((uint32_t *)(uintptr_t)(e->value_p)) = value;
    jx_set_updated(e);
}

/**
 * @brief Set a signed 32-bit integer value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Signed 32-bit integer value to set.
 */
static inline void jx_set_i32(JX_ELEMENT *e, int32_t value)
{
    *((int32_t *)(uintptr_t)(e->value_p)) = value;
    jx_set_updated(e);
}

/**
 * @brief Set an unsigned 64-bit integer value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Unsigned 64-bit integer value to set.
 */
static inline void jx_set_u64(JX_ELEMENT *e, uint64_t value)
{
    *((uint64_t *)(uintptr_t)(e->value_p)) = value;
    jx_set_updated(e);
}

/**
 * @brief Set a signed 64-bit integer value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Signed 64-bit integer value to set.
 */
static inline void jx_set_i64(JX_ELEMENT *e, int64_t value)
{
    *((int64_t *)(uintptr_t)(e->value_p)) = value;
    jx_set_updated(e);
}

/**
 * @brief Set a string value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Null-terminated string to copy.
 */
static inline void jx_set_string(JX_ELEMENT *e, const char *value)
{
    size_t capacity = (e->value_capacity != 0U) ? e->value_capacity : JX_PROPERTY_MAX_SIZE;
    char *destination = (char *)(uintptr_t)(e->value_p);

    strncpy(destination, value, capacity);
    destination[capacity - 1U] = '\0';
    jx_set_updated(e);
}

/**
 * @brief Clear the string value (set to empty string) and mark as updated.
 *
 * @param e Pointer to JX_ELEMENT.
 */
static inline void jx_set_string_empty(JX_ELEMENT *e)
{
    ((char *)(uintptr_t)(e->value_p))[0] = '\0';
    jx_set_updated(e);
}

#ifdef __cplusplus
}
#endif

#endif /* JX_INTERNAL_H */
