/**************************************************************************/
/*                                                                        */
/*  @file jx_types.h                                                      */
/*  @brief Core Types and Constants for JsonX                             */
/*                                                                        */
/*  Defines public enums, mapping types, and user-facing macros.          */
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
#include <stddef.h>
#include <stdint.h>

#include "jx_config.h"
#ifdef JX_USE_THREADX
#include "tx_api.h"
#endif

/**************************************************************************/
/*                                                                        */
/*  Data Type Definitions                                                 */
/*                                                                        */
/**************************************************************************/

/** JsonX parser lifecycle state. */
typedef enum
{
    JX_RESET = 0,
    JX_INITIALIZED
} JX_STATE;

/** General JsonX status code. */
typedef enum
{
    JX_SUCCESS = 0,
    JX_ERROR
} JX_STATUS;

/** JSON parse behavior mode. */
typedef enum
{
    JX_MODE_RELAXED = 0,
    JX_MODE_STRICT
} JX_PARSE_MODE;

/** Custom allocation hook table used by custom allocator mode. */
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

/** JSON value type represented by a `JX_ELEMENT`. */
typedef enum
{
    JX_INVALID = 0,
    JX_NULL,
    JX_BOOLEAN,
    JX_NUMBER,
    JX_U32,
    JX_I32,
    JX_U64,
    JX_I64,
    JX_STRING,
    JX_ARRAY,
    JX_OBJECT
} JX_ELEMENT_TYPE;

/** Indicates whether a mapped element was updated during parsing. */
typedef enum
{
    JX_ELEMENT_NOT_UPDATED = 0,
    JX_ELEMENT_UPDATED
} JX_ELEMENT_STATUS;

/** JSON output formatting mode. */
typedef enum
{
    JX_MINIFIED = 0,
    JX_FORMATTED
} JX_FORMAT;

/** Declarative mapping between a JSON node and caller-owned C storage. */
typedef struct json_element_s
{
    char                    property[JX_PROPERTY_MAX_SIZE];
    JX_ELEMENT_TYPE         type;
    uint8_t                 value_len;
    uint8_t                 value_capacity;
    const void             *value_p;
    JX_ELEMENT_STATUS       status;
    struct json_element_s  *element;
    uint16_t                element_size;
} JX_ELEMENT;

/**************************************************************************/
/*                                                                        */
/*  Mapping Macros                                                        */
/*                                                                        */
/**************************************************************************/
#define JX_STRING_PTR(_value_p) \
    { .type = JX_STRING, .value_p = (void*)(_value_p), .value_capacity = JX_PROPERTY_MAX_SIZE }

#define JX_STRING_REF(_value_p) \
    { .type = JX_STRING, .value_p = (void*)&(_value_p), .value_capacity = JX_PROPERTY_MAX_SIZE }

#define JX_STRING_PTR_N(_value_p, _capacity) \
    { .type = JX_STRING, .value_p = (void*)(_value_p), .value_capacity = (_capacity) }

#define JX_STRING_REF_N(_value_p, _capacity) \
    { .type = JX_STRING, .value_p = (void*)&(_value_p), .value_capacity = (_capacity) }

#define JX_STRING_BUFFER(_buffer) \
    { .type = JX_STRING, .value_p = (void*)(_buffer), .value_capacity = sizeof(_buffer) }

#define JX_BOOLEAN_VAL(_value_p) \
    { .type = JX_BOOLEAN, .value_p = &_value_p }

#define JX_U32_VAL(_value_p) \
    { .type = JX_U32, .value_p = &_value_p }

#define JX_I32_VAL(_value_p) \
    { .type = JX_I32, .value_p = &_value_p }

#define JX_U64_VAL(_value_p) \
    { .type = JX_U64, .value_p = &_value_p }

#define JX_I64_VAL(_value_p) \
    { .type = JX_I64, .value_p = &_value_p }

#define JX_ARRAY_VAL(_element) \
    { .type = JX_ARRAY, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]), .value_capacity = sizeof(_element) / sizeof(_element[0]) }

#define JX_OBJECT_VAL(_element) \
    { .type = JX_OBJECT, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]) }

#define JX_OBJECT_EMPTY \
    { .type = JX_OBJECT, .element = NULL, .value_len = 1 }

#define JX_PROPERTY_STRING(_property, _value_p) \
    { .property = _property, .type = JX_STRING, .value_p = _value_p, .value_capacity = JX_PROPERTY_MAX_SIZE }

#define JX_PROPERTY_STRING_N(_property, _value_p, _capacity) \
    { .property = _property, .type = JX_STRING, .value_p = _value_p, .value_capacity = (_capacity) }

#define JX_PROPERTY_STRING_BUFFER(_property, _buffer) \
    { .property = _property, .type = JX_STRING, .value_p = _buffer, .value_capacity = sizeof(_buffer) }

#define JX_PROPERTY_BOOLEAN(_property, _value_p) \
    { .property = _property, .type = JX_BOOLEAN, .value_p = &_value_p }

#define JX_PROPERTY_U32(_property, _value_p) \
    { .property = _property, .type = JX_U32, .value_p = &_value_p }

#define JX_PROPERTY_I32(_property, _value_p) \
    { .property = _property, .type = JX_I32, .value_p = &_value_p }

#define JX_PROPERTY_U64(_property, _value_p) \
    { .property = _property, .type = JX_U64, .value_p = &_value_p }

#define JX_PROPERTY_I64(_property, _value_p) \
    { .property = _property, .type = JX_I64, .value_p = &_value_p }

#define JX_PROPERTY_NULL(_property) \
    { .property = _property, .type = JX_NULL }

#if JX_ENABLE_DOUBLE
#define JX_NUMBER_VAL(_value_p) \
    { .type = JX_NUMBER, .value_p = &_value_p }

#define JX_PROPERTY_NUMBER(_property, _value_p) \
    { .property = _property, .type = JX_NUMBER, .value_p = &_value_p }
#endif

#define JX_PROPERTY_ARRAY(_property, _element) \
    { .property = _property, .type = JX_ARRAY, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]), .value_capacity = sizeof(_element) / sizeof(_element[0]) }

#define JX_PROPERTY_OBJECT(_property, _element) \
    { .property = _property, .type = JX_OBJECT, .element = _element, .value_len = sizeof(_element) / sizeof(_element[0]) }

#define JX_PROPERTY_OBJECT_EMPTY(_property) \
    { .property = _property, .type = JX_OBJECT, .element = NULL, .value_len = 1 }


#ifdef __cplusplus
}
#endif

#endif /* JX_TYPES_H */
