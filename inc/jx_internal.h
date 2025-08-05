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
#include <string.h>

/**************************************************************************/
/*                                                                        */
/*  Internal Macros                                                       */
/*                                                                        */
/**************************************************************************/

/* Return false if pointer is NULL */
#define JX_RETURN_IF_NULL(p)        \
    if ((p) == NULL)                \
    {                               \
        return false;               \
    }

/**************************************************************************/
/*                                                                        */
/*  Inline Helpers                                                        */
/*                                                                        */
/**************************************************************************/

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
 * @brief Set a string value in the element and mark as updated.
 *
 * @param e     Pointer to JX_ELEMENT.
 * @param value Null-terminated string to copy.
 */
static inline void jx_set_string(JX_ELEMENT *e, const char *value)
{
    strncpy((char *)(uintptr_t)(e->value_p), value, JX_PROPERTY_MAX_SIZE);
    jx_set_updated(e);
}

/**
 * @brief Set a uint32_t value to zero and mark as updated.
 *
 * @param e Pointer to JX_ELEMENT.
 */
static inline void jx_set_null_u32(JX_ELEMENT *e)
{
    *((uint32_t *)(uintptr_t)(e->value_p)) = 0;
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
