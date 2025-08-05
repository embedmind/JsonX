/**************************************************************************/
/*                                                                        */
/*  @file jx_types.h                                                      */
/*  @brief Core Types and Constants for JsonX                             */
/*                                                                        */
/*  Defines enums, macros, and internal structures used by the library.   */
/*  This file is safe to include in user code.                            */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/


#ifndef JX_TYPES_H
#define JX_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************/
/*                                                                        */
/*  Include Files                                                         */
/*                                                                        */
/**************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include "cJSON.h"

#include "jx_config.h"
#ifdef JX_USE_THREADX
#include "tx_api.h"
#endif
#ifdef JX_USE_BAREMETAL
#include "jx_static_allocator.h"
#endif

/**************************************************************************/
/*                                                                        */
/*  Data Type Definitions                                                 */
/*                                                                        */
/**************************************************************************/

/* JSON parser internal state */
typedef enum
{
    JX_RESET = 0,
    JX_INITIALIZED
} JX_STATE;

/* General status codes */
typedef enum
{
    JX_SUCCESS = 0,
    JX_ERROR
} JX_STATUS;

/* Strict mode parsing behavior */
typedef enum
{
    JX_MODE_RELAXED = 0,
    JX_MODE_STRICT
} JX_PARSE_MODE;

/* Memory allocation hooks */
typedef struct
{
    void *(*malloc_fn)(size_t sz);
    void (*free_fn)(void *ptr);
#ifndef JX_USE_THREADX
#ifndef JX_USE_HEAP_BAREMETAL
    void(*reset_fn)(void);
#endif
#endif
} JX_HOOKS;

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

/* JSON element type */
typedef enum
{
    JX_INVALID = 0,
    JX_NULL,
    JX_BOOLEAN,
    JX_NUMBER,
    JX_STRING,
    JX_ARRAY,
    JX_OBJECT
} JX_ELEMENT_TYPE;

/* Element update status */
typedef enum
{
    JX_ELEMENT_NOT_UPDATED = 0,
    JX_ELEMENT_UPDATED
} JX_ELEMENT_STATUS;

/* Formatting options */
typedef enum
{
    JX_MINIFIED = 0,
    JX_FORMATTED
} JX_FORMAT;

/* JSON element definition */
typedef struct json_element_s
{
    char                    property[JX_PROPERTY_MAX_SIZE];
    JX_ELEMENT_TYPE         type;
    uint8_t                 value_len;
    const void             *value_p;
    JX_ELEMENT_STATUS       status;
    struct json_element_s  *element;
    uint16_t                element_size;
} JX_ELEMENT;

/**************************************************************************/
/*                                                                        */
/*  User Macros                                                           */
/*                                                                        */
/**************************************************************************/
#define JX_STRING_PTR(_value_p) \
    { .type = JX_STRING, .value_p = (void*)(_value_p) }

#define JX_STRING_REF(_value_p) \
    { .type = JX_STRING, .value_p = (void*)&(_value_p) }

#define JX_BOOLEAN_VAL(_value_p) \
    { .type = JX_BOOLEAN, .value_p = &_value_p }

#define JX_NUMBER_VAL(_value_p) \
    { .type = JX_NUMBER, .value_p = &_value_p }

#define JX_ARRAY_VAL(_element) \
    { .type = JX_ARRAY, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]) }

#define JX_OBJECT_VAL(_element) \
    { .type = JX_OBJECT, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]) }

#define JX_OBJECT_EMPTY \
    { .type = JX_OBJECT, .element = NULL, .value_len = 1 }

#define JX_PROPERTY_STRING(_property, _value_p) \
    { .property = _property, .type = JX_STRING, .value_p = _value_p }

#define JX_PROPERTY_BOOLEAN(_property, _value_p) \
    { .property = _property, .type = JX_BOOLEAN, .value_p = &_value_p }

#define JX_PROPERTY_NUMBER(_property, _value_p) \
    { .property = _property, .type = JX_NUMBER, .value_p = &_value_p }

#define JX_PROPERTY_ARRAY(_property, _element) \
    { .property = _property, .type = JX_ARRAY, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]) }

#define JX_PROPERTY_OBJECT(_property, _element) \
    { .property = _property, .type = JX_OBJECT, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]) }

#define JX_PROPERTY_OBJECT_EMPTY(_property) \
    { .property = _property, .type = JX_OBJECT, .element = NULL, .value_len = 1 }


/**************************************************************************/
/*                                                                        */
/*  How to Use                                                            */
/*                                                                        */
/*  - Include this header in modules that define or parse JSON elements. */
/*  - Define JSON schema using JX_PROPERTY_* macros.                     */
/*  - Use with jx_parse(), jx_struct_to_object(), jx_object_to_struct(). */
/*                                                                        */
/**************************************************************************/

#ifdef __cplusplus
}
#endif

#endif /* JX_TYPES_H */
