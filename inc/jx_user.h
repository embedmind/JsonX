/**************************************************************************/
/*                                                                        */
/*  @file jx_user.h                                                       */
/*  @brief User-Defined Macros for JsonX                                  */
/*                                                                        */
/*  Contains helper macros that simplify array/object layout definitions  */
/*  for the JX_ELEMENT[] model. Can be customized freely by the user.     */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

/**
 * @file jx_user.h
 * @brief Preprocessor helpers for user-defined composite macros.
 *
 * This file provides macros for constructing common array and object patterns
 * using `JX_ELEMENT` arrays. It is useful when mapping fixed-size C structures
 * into JSON layouts via reusable macro patterns.
 *
 * The macros include:
 * - Fixed-size number arrays
 * - Fixed-size string arrays
 * - Object arrays (arrays of nested JX_ELEMENT groups)
 *
 * @note This file is intended for user-level customization only. Modifying or
 *       extending this file does not affect core library logic.
 *
 * @section HOW_TO_USE How to Use
 *
 * Example 1: Define a 2-element numeric array
 * @code
 * double temperature[2];
 * JX_PROPERTY_NUMBER_ARRAY_2(temperature_array, temperature);
 * @endcode
 *
 * Example 2: Define a 3-element string array
 * @code
 * char skills[3][32];
 * JX_PROPERTY_STRING_ARRAY_3(skills_array, skills);
 * @endcode
 *
 * Example 3: Define a full object with nested arrays and objects
 * @code
 * typedef struct {
 *     char FirstName[32];
 *     char LastName[32];
 *     double BirthDate[3];
 *     char Skills[3][32];
 *     struct {
 *         char City[32];
 *         char ZipCode[10];
 *     } Addresses[2];
 * } Person_t;
 *
 * Person_t John;
 *
 * JX_PROPERTY_NUMBER_ARRAY_3(BirthDate_array, John.BirthDate);
 * JX_PROPERTY_STRING_ARRAY_3(Skills_array, John.Skills);
 *
 * JX_ELEMENT address0[] = {
 *     JX_PROPERTY_STRING("city", John.Addresses[0].City),
 *     JX_PROPERTY_STRING("zip", John.Addresses[0].ZipCode)
 * };
 *
 * JX_ELEMENT address1[] = {
 *     JX_PROPERTY_STRING("city", John.Addresses[1].City),
 *     JX_PROPERTY_STRING("zip", John.Addresses[1].ZipCode)
 * };
 *
 * JX_PROPERTY_OBJECT_ARRAY_2(addresses_array, address0, address1);
 *
 * JX_ELEMENT JSON_Person_body[] = {
 *     JX_PROPERTY_STRING("FirstName", John.FirstName),
 *     JX_PROPERTY_STRING("LastName", John.LastName),
 *     JX_PROPERTY_ARRAY("BirthDate", BirthDate_array),
 *     JX_PROPERTY_ARRAY("Skills", Skills_array),
 *     JX_PROPERTY_ARRAY("Addresses", addresses_array),
 * };
 *
 * const size_t JSON_Person_body_size = sizeof(JSON_Person_body) / sizeof(JSON_Person_body[0]);
 * JX_ELEMENT JSON_Person[1] = { JX_PROPERTY_OBJECT("person", JSON_Person_body) };
 * const size_t JSON_Person_size = sizeof(JSON_Person) / sizeof(JSON_Person[0]);
 * @endcode
 */




#ifndef JX_USER_H
#define JX_USER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "jx_types.h"

/**************************************************************************/
/* Number Array Property Macros                                          */
/**************************************************************************/

/**
 * @brief Define a JSON property with a number array of size 2
 */
#define JX_PROPERTY_NUMBER_ARRAY_2(name, array)                            \
    JX_ELEMENT name[2] = {                                                 \
        JX_NUMBER_VAL(array[0]),                                           \
        JX_NUMBER_VAL(array[1]),                                           \
    }

/**
 * @brief Define a JSON property with a number array of size 3
 */
#define JX_PROPERTY_NUMBER_ARRAY_3(name, array)                            \
    JX_ELEMENT name[3] = {                                                 \
        JX_NUMBER_VAL(array[0]),                                           \
        JX_NUMBER_VAL(array[1]),                                           \
        JX_NUMBER_VAL(array[2]),                                           \
    }

/**
 * @brief Define a JSON property with a number array of size 4
 */
#define JX_PROPERTY_NUMBER_ARRAY_4(name, array)                            \
    JX_ELEMENT name[4] = {                                                 \
        JX_NUMBER_VAL(array[0]),                                           \
        JX_NUMBER_VAL(array[1]),                                           \
        JX_NUMBER_VAL(array[2]),                                           \
        JX_NUMBER_VAL(array[3])                                            \
    }

/**************************************************************************/
/* String Array Property Macros                                          */
/**************************************************************************/

/**
 * @brief Define a JSON property with a string array of size 2
 */
#define JX_PROPERTY_STRING_ARRAY_2(name, array)                            \
    JX_ELEMENT name[2] = {                                                 \
        JX_STRING_PTR(&(array)[0]),                                        \
        JX_STRING_PTR(&(array)[1])                                         \
    }

/**
 * @brief Define a JSON property with a string array of size 3
 */
#define JX_PROPERTY_STRING_ARRAY_3(name, array)                            \
    JX_ELEMENT name[3] = {                                                 \
        JX_STRING_PTR(&(array)[0]),                                        \
        JX_STRING_PTR(&(array)[1]),                                        \
        JX_STRING_PTR(&(array)[2])                                         \
    }

/**************************************************************************/
/* Object Array Property Macros                                          */
/**************************************************************************/

/**
 * @brief Declare a 2-element object array using two nested object element arrays.
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
#ifdef __cplusplus
}
#endif

#endif /* JX_USER_H */
