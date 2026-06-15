/**************************************************************************/
/*                                                                        */
/*  @file jx_user.h                                                       */
/*  @brief User-facing JsonX mapping helper macros                        */
/*                                                                        */
/*  Contains helper macros that simplify fixed-size array and object      */
/*  layout declarations for the JX_ELEMENT[] mapping model.               */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#ifndef JX_USER_H
#define JX_USER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "jx_types.h"

/**************************************************************************/
/*  Number Array Property Macros                                          */
/**************************************************************************/

#if JX_ENABLE_DOUBLE
/**
 * @brief Declare a two-element numeric JSON array mapping.
 */
#define JX_PROPERTY_NUMBER_ARRAY_2(name, array)                            \
    JX_ELEMENT name[2] = {                                                 \
        JX_NUMBER_VAL(array[0]),                                           \
        JX_NUMBER_VAL(array[1]),                                           \
    }

/**
 * @brief Declare a three-element numeric JSON array mapping.
 */
#define JX_PROPERTY_NUMBER_ARRAY_3(name, array)                            \
    JX_ELEMENT name[3] = {                                                 \
        JX_NUMBER_VAL(array[0]),                                           \
        JX_NUMBER_VAL(array[1]),                                           \
        JX_NUMBER_VAL(array[2]),                                           \
    }

/**
 * @brief Declare a four-element numeric JSON array mapping.
 */
#define JX_PROPERTY_NUMBER_ARRAY_4(name, array)                            \
    JX_ELEMENT name[4] = {                                                 \
        JX_NUMBER_VAL(array[0]),                                           \
        JX_NUMBER_VAL(array[1]),                                           \
        JX_NUMBER_VAL(array[2]),                                           \
        JX_NUMBER_VAL(array[3])                                            \
    }
#endif

/**
 * @brief Declare a two-element unsigned 32-bit integer JSON array mapping.
 */
#define JX_PROPERTY_U32_ARRAY_2(name, array)                               \
    JX_ELEMENT name[2] = {                                                 \
        JX_U32_VAL(array[0]),                                              \
        JX_U32_VAL(array[1]),                                              \
    }

/**
 * @brief Declare a three-element unsigned 32-bit integer JSON array mapping.
 */
#define JX_PROPERTY_U32_ARRAY_3(name, array)                               \
    JX_ELEMENT name[3] = {                                                 \
        JX_U32_VAL(array[0]),                                              \
        JX_U32_VAL(array[1]),                                              \
        JX_U32_VAL(array[2]),                                              \
    }

/**
 * @brief Declare a four-element unsigned 32-bit integer JSON array mapping.
 */
#define JX_PROPERTY_U32_ARRAY_4(name, array)                               \
    JX_ELEMENT name[4] = {                                                 \
        JX_U32_VAL(array[0]),                                              \
        JX_U32_VAL(array[1]),                                              \
        JX_U32_VAL(array[2]),                                              \
        JX_U32_VAL(array[3])                                               \
    }

/**
 * @brief Declare a two-element unsigned 64-bit integer JSON array mapping.
 */
#define JX_PROPERTY_U64_ARRAY_2(name, array)                               \
    JX_ELEMENT name[2] = {                                                 \
        JX_U64_VAL(array[0]),                                              \
        JX_U64_VAL(array[1]),                                              \
    }

/**
 * @brief Declare a three-element unsigned 64-bit integer JSON array mapping.
 */
#define JX_PROPERTY_U64_ARRAY_3(name, array)                               \
    JX_ELEMENT name[3] = {                                                 \
        JX_U64_VAL(array[0]),                                              \
        JX_U64_VAL(array[1]),                                              \
        JX_U64_VAL(array[2]),                                              \
    }

/**
 * @brief Declare a four-element unsigned 64-bit integer JSON array mapping.
 */
#define JX_PROPERTY_U64_ARRAY_4(name, array)                               \
    JX_ELEMENT name[4] = {                                                 \
        JX_U64_VAL(array[0]),                                              \
        JX_U64_VAL(array[1]),                                              \
        JX_U64_VAL(array[2]),                                              \
        JX_U64_VAL(array[3])                                               \
    }

/**************************************************************************/
/*  String Array Property Macros                                          */
/**************************************************************************/

/**
 * @brief Declare a two-element string JSON array mapping.
 */
#define JX_PROPERTY_STRING_ARRAY_2(name, array)                            \
    JX_ELEMENT name[2] = {                                                 \
        JX_STRING_BUFFER((array)[0]),                                      \
        JX_STRING_BUFFER((array)[1])                                       \
    }

/**
 * @brief Declare a three-element string JSON array mapping.
 */
#define JX_PROPERTY_STRING_ARRAY_3(name, array)                            \
    JX_ELEMENT name[3] = {                                                 \
        JX_STRING_BUFFER((array)[0]),                                      \
        JX_STRING_BUFFER((array)[1]),                                      \
        JX_STRING_BUFFER((array)[2])                                       \
    }

/**************************************************************************/
/*  Object And Dynamic-Count Array Macros                                 */
/**************************************************************************/

/**
 * @brief Create an object array item with an explicit element count.
 *
 * Use this when the nested object is stored in a fixed backing array but the
 * valid element count is known from a schema constant instead of sizeof().
 */
#define JX_OBJECT_VAL_N(object, count)                                     \
    { .type = JX_OBJECT, .element = (object), .value_len = (count) }

/**
 * @brief Create an array property with an explicit element count.
 *
 * Use this when the array backing storage is larger than the current logical
 * JSON array length.
 */
#define JX_PROPERTY_ARRAY_N(property_name, array, count)                   \
    { .property = property_name, .type = JX_ARRAY, .element = (array), .value_len = (count), .value_capacity = (count) }

/**
 * @brief Declare a two-element object array from two nested objects.
 *
 * @param name      The name of the resulting JX_ELEMENT[] array
 * @param object0  First nested object (JX_ELEMENT[])
 * @param object1  Second nested object (JX_ELEMENT[])
 */
#define JX_PROPERTY_OBJECT_ARRAY_2(name, object0, object1)     \
    JX_ELEMENT name[2] =                                       \
    {                                                          \
        JX_OBJECT_VAL(object0),                                \
        JX_OBJECT_VAL(object1),                                \
    }

/**
 * @brief Declare a six-element object array from six nested objects.
 *
 * @param name      The name of the resulting JX_ELEMENT[] array
 * @param object0  First nested object (JX_ELEMENT[])
 * @param object1  Second nested object (JX_ELEMENT[])
 * @param object2  Third nested object (JX_ELEMENT[])
 * @param object3  Fourth nested object (JX_ELEMENT[])
 * @param object4  Fifth nested object (JX_ELEMENT[])
 * @param object5  Sixth nested object (JX_ELEMENT[])
 */
#define JX_PROPERTY_OBJECT_ARRAY_6(name, object0, object1, object2, object3, object4, object5) \
    JX_ELEMENT name[6] =                                                                       \
    {                                                                                          \
        JX_OBJECT_VAL(object0),                                                                \
        JX_OBJECT_VAL(object1),                                                                \
        JX_OBJECT_VAL(object2),                                                                \
        JX_OBJECT_VAL(object3),                                                                \
        JX_OBJECT_VAL(object4),                                                                \
        JX_OBJECT_VAL(object5),                                                                \
    }
#ifdef __cplusplus
}
#endif

#endif /* JX_USER_H */
