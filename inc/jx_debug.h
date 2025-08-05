/**************************************************************************/
/*                                                                        */
/*  @file jx_debug.h                                                      */
/*  @brief Debug and Logging Utilities for JsonX                          */
/*                                                                        */
/*  This file is part of the JsonX Library.                               */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/


#ifndef JX_DEBUG_H
#define JX_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/*                                                                        */
/*  Include Files                                                         */
/*                                                                        */
/**************************************************************************/

#include "jx_types.h"
#include <stdio.h>
#include <stdarg.h>

/**************************************************************************/
/*                                                                        */
/*  Logging Macros                                                        */
/*                                                                        */
/**************************************************************************/

/* Logging macro - override if needed */
#ifndef jx_log
#define jx_log    printf
#endif

/* Error check and jump to cleanup if object is NULL */
#define IF_JX_ERROR_EXIT(object)                              \
    if ((object) == NULL)                                     \
    {                                                         \
        const char *error_ptr = cJSON_GetErrorPtr();          \
        if (error_ptr != NULL)                                \
        {                                                     \
            jx_log("Error before: %s\n", error_ptr);          \
        }                                                     \
        goto end;                                             \
    }

/**************************************************************************/
/*                                                                        */
/*  Debug Helpers                                                         */
/*                                                                        */
/**************************************************************************/

 /**
 * @brief Dump contents of JX_ELEMENT tree (debug only)
 * @note This is a debug helper, declared inline to avoid linkage issues.
 *
 * @param elements  Pointer to array of JX_ELEMENT.
 * @param count     Number of elements in the array.
 */
static inline void jx_dump_structure(const JX_ELEMENT *elements, size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const char *status_str = (elements[i].status == JX_ELEMENT_UPDATED)
                               ? "updated"
                               : "not updated";

        jx_log("[%02u] %s (%s): ",
               (unsigned)i,
               elements[i].property[0] ? elements[i].property : "<no name>",
               status_str);

        switch (elements[i].type)
        {
            case JX_STRING:
                jx_log("\"%s\"\n", (const char *)elements[i].value_p);
                break;

            case JX_NUMBER:
                jx_log("%ld\n", *(const long *)elements[i].value_p);
                break;

            case JX_BOOLEAN:
                jx_log("%s\n", (*(const bool *)elements[i].value_p) ? "true" : "false");
                break;

            case JX_ARRAY:
            case JX_OBJECT:
                jx_log("[nested %lu elements]\n", (unsigned long)elements[i].value_len);
                break;

            default:
                jx_log("(type unsupported for print)\n");
                break;
        }
    }
}

#ifdef __cplusplus
}
#endif

#endif /* JX_DEBUG_H */
