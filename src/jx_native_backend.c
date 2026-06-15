/**************************************************************************/
/*                                                                        */
/*  @file jx_native_backend.c                                             */
/*  @brief Native JsonX backend adapter                                   */
/*                                                                        */
/*  This backend implements direct parse/write over JX_ELEMENT mappings.  */
/*  It intentionally does not use strtok-style tokenizers.                */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#include "../private/jx_backend.h"
#include "../private/jx_internal.h"

#include <string.h>

typedef struct
{
    const char *start;
    const char *cursor;
    const char *error;
    uint8_t depth;
} JX_NATIVE_READER;

typedef struct
{
    char *buffer;
    size_t size;
    size_t pos;
    bool formatted;
    bool failed;
} JX_NATIVE_WRITER;

static const char *jx_native_error_ptr = NULL;

static void jx_native_writer_putc(JX_NATIVE_WRITER *writer, char c);
static void jx_native_writer_puts(JX_NATIVE_WRITER *writer, const char *text);
static bool jx_native_set_error(JX_NATIVE_READER *reader);
static bool jx_native_write_elements(JX_NATIVE_WRITER *writer,
                                     JX_ELEMENT *elements,
                                     size_t element_count,
                                     uint8_t depth,
                                     bool object_context);
static bool jx_native_write_element_value(JX_NATIVE_WRITER *writer, JX_ELEMENT *element, uint8_t depth);
static bool jx_native_enter_container(JX_NATIVE_READER *reader);
static bool jx_native_skip_value(JX_NATIVE_READER *reader);
static bool jx_native_skip_string(JX_NATIVE_READER *reader);
static bool jx_native_skip_number(JX_NATIVE_READER *reader);
static bool jx_native_skip_object(JX_NATIVE_READER *reader);
static bool jx_native_skip_array(JX_NATIVE_READER *reader);
static bool jx_native_parse_string_into_buffer(JX_NATIVE_READER *reader, char *buffer, size_t buffer_size);
#if JX_ENABLE_DOUBLE
static bool jx_native_parse_number_value(JX_NATIVE_READER *reader, double *value);
#endif
static bool jx_native_parse_u32_value(JX_NATIVE_READER *reader, uint32_t *value);
static bool jx_native_parse_i32_value(JX_NATIVE_READER *reader, int32_t *value);
static bool jx_native_parse_u64_value(JX_NATIVE_READER *reader, uint64_t *value);
static bool jx_native_parse_i64_value(JX_NATIVE_READER *reader, int64_t *value);
static JX_ELEMENT *jx_native_find_element(JX_ELEMENT *elements, size_t element_count, const char *property);
static JX_STATUS jx_native_handle_type_mismatch(JX_ELEMENT *element, JX_PARSE_MODE mode);
static JX_STATUS jx_native_parse_element_value(JX_NATIVE_READER *reader, JX_ELEMENT *element, JX_PARSE_MODE mode);
static JX_STATUS jx_native_parse_array_into_elements(JX_NATIVE_READER *reader, JX_ELEMENT *element, JX_PARSE_MODE mode);
static JX_STATUS jx_native_parse_object_into_elements(JX_NATIVE_READER *reader,
                                                      JX_ELEMENT *elements,
                                                      size_t element_count,
                                                      JX_PARSE_MODE mode);

static void jx_native_skip_ws(JX_NATIVE_READER *reader)
{
    while ((reader->cursor != NULL) && (*reader->cursor != '\0'))
    {
        if ((*reader->cursor == ' ') || (*reader->cursor == '\t') ||
            (*reader->cursor == '\n') || (*reader->cursor == '\r'))
        {
            reader->cursor++;
            continue;
        }

#if JX_ENABLE_JSON_COMMENTS
        if ((reader->cursor[0] == '/') && (reader->cursor[1] == '/'))
        {
            reader->cursor += 2;
            while ((*reader->cursor != '\0') &&
                   (*reader->cursor != '\n') &&
                   (*reader->cursor != '\r'))
            {
                reader->cursor++;
            }
            continue;
        }

        if ((reader->cursor[0] == '/') && (reader->cursor[1] == '*'))
        {
            bool closed = false;

            reader->cursor += 2;
            while (reader->cursor[0] != '\0')
            {
                if ((reader->cursor[0] == '*') && (reader->cursor[1] == '/'))
                {
                    reader->cursor += 2;
                    closed = true;
                    break;
                }

                reader->cursor++;
            }

            if (!closed)
            {
                (void)jx_native_set_error(reader);
            }
            continue;
        }
#endif

        break;
    }
}

static bool jx_native_set_error(JX_NATIVE_READER *reader)
{
    if ((reader != NULL) && (reader->error == NULL))
    {
        reader->error = reader->cursor;
    }

    return false;
}

static bool jx_native_match_literal(JX_NATIVE_READER *reader, const char *literal)
{
    size_t length = strlen(literal);

    if (strncmp(reader->cursor, literal, length) != 0)
    {
        return jx_native_set_error(reader);
    }

    reader->cursor += length;
    return true;
}

static bool jx_native_enter_container(JX_NATIVE_READER *reader)
{
    if (reader == NULL)
    {
        return false;
    }

    if (reader->depth >= JX_MAX_NESTING_LEVEL)
    {
        return jx_native_set_error(reader);
    }

    reader->depth++;
    return true;
}

static int jx_native_hex_value(char c)
{
    if ((c >= '0') && (c <= '9'))
    {
        return c - '0';
    }
    if ((c >= 'a') && (c <= 'f'))
    {
        return c - 'a' + 10;
    }
    if ((c >= 'A') && (c <= 'F'))
    {
        return c - 'A' + 10;
    }

    return -1;
}

static bool jx_native_skip_string(JX_NATIVE_READER *reader)
{
    if ((reader == NULL) || (*reader->cursor != '"'))
    {
        return jx_native_set_error(reader);
    }

    reader->cursor++;
    while (*reader->cursor != '\0')
    {
        char c = *reader->cursor++;

        if (c == '"')
        {
            return true;
        }

        if ((unsigned char)c < 0x20U)
        {
            reader->cursor--;
            return jx_native_set_error(reader);
        }

        if (c == '\\')
        {
            c = *reader->cursor++;
            switch (c)
            {
            case '"':
            case '\\':
            case '/':
            case 'b':
            case 'f':
            case 'n':
            case 'r':
            case 't':
                break;

            case 'u':
                for (uint8_t i = 0U; i < 4U; ++i)
                {
                    if (jx_native_hex_value(*reader->cursor++) < 0)
                    {
                        return jx_native_set_error(reader);
                    }
                }
                break;

            default:
                reader->cursor--;
                return jx_native_set_error(reader);
            }
        }
    }

    return jx_native_set_error(reader);
}

static bool jx_native_parse_string_into_buffer(JX_NATIVE_READER *reader, char *buffer, size_t buffer_size)
{
    char *write;
    size_t remaining;

    if ((reader == NULL) || (buffer == NULL) || (buffer_size == 0U) || (*reader->cursor != '"'))
    {
        return jx_native_set_error(reader);
    }

    reader->cursor++;
    write = buffer;
    remaining = buffer_size;

    while (*reader->cursor != '\0')
    {
        char c = *reader->cursor++;

        if (c == '"')
        {
            *write = '\0';
            return true;
        }

        if ((unsigned char)c < 0x20U)
        {
            reader->cursor--;
            return jx_native_set_error(reader);
        }

        if (c == '\\')
        {
            c = *reader->cursor++;
            switch (c)
            {
            case '"':
            case '\\':
            case '/':
                break;

            case 'b':
                c = '\b';
                break;

            case 'f':
                c = '\f';
                break;

            case 'n':
                c = '\n';
                break;

            case 'r':
                c = '\r';
                break;

            case 't':
                c = '\t';
                break;

            case 'u':
            {
                int code = 0;

                for (uint8_t i = 0U; i < 4U; ++i)
                {
                    int hex = jx_native_hex_value(*reader->cursor++);
                    if (hex < 0)
                    {
                        return jx_native_set_error(reader);
                    }
                    code = (code << 4) | hex;
                }
                if ((code <= 0x1F) || (code > 0x7F))
                {
                    return jx_native_set_error(reader);
                }
                c = (char)code;
                break;
            }

            default:
                reader->cursor--;
                return jx_native_set_error(reader);
            }
        }

        if (remaining <= 1U)
        {
            return jx_native_set_error(reader);
        }

        *write++ = c;
        remaining--;
    }

    return jx_native_set_error(reader);
}

static bool jx_native_skip_number(JX_NATIVE_READER *reader)
{
    const char *cursor;
    bool has_digit = false;

    if (reader == NULL)
    {
        return false;
    }

    cursor = reader->cursor;
    if (*cursor == '-')
    {
        cursor++;
    }

    if (*cursor == '0')
    {
        has_digit = true;
        cursor++;
        if ((*cursor >= '0') && (*cursor <= '9'))
        {
            reader->cursor = cursor;
            return jx_native_set_error(reader);
        }
    }
    else
    {
        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            has_digit = true;
            cursor++;
        }
    }

    if (!has_digit)
    {
        reader->cursor = cursor;
        return jx_native_set_error(reader);
    }

    if (*cursor == '.')
    {
        bool has_fraction_digit = false;

        cursor++;
        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            has_fraction_digit = true;
            cursor++;
        }

        if (!has_fraction_digit)
        {
            reader->cursor = cursor;
            return jx_native_set_error(reader);
        }
    }

    if ((*cursor == 'e') || (*cursor == 'E'))
    {
        bool has_exponent_digit = false;
        bool exponent_overflow = false;
        int exponent = 0;

        cursor++;
        if ((*cursor == '+') || (*cursor == '-'))
        {
            cursor++;
        }

        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            has_exponent_digit = true;
            if (exponent < 308)
            {
                exponent = (exponent * 10) + (*cursor - '0');
            }
            else
            {
                exponent_overflow = true;
            }
            cursor++;
        }

        if ((!has_exponent_digit) || exponent_overflow || (exponent > 308))
        {
            reader->cursor = cursor;
            return jx_native_set_error(reader);
        }
    }

    reader->cursor = cursor;
    return true;
}

#if JX_ENABLE_DOUBLE
static bool jx_native_parse_number_value(JX_NATIVE_READER *reader, double *value)
{
    const char *cursor;
    bool negative = false;
    bool has_digit = false;
    double parsed = 0.0;

    if ((reader == NULL) || (value == NULL))
    {
        return false;
    }

    cursor = reader->cursor;
    if (*cursor == '-')
    {
        negative = true;
        cursor++;
    }

    if (*cursor == '0')
    {
        has_digit = true;
        cursor++;
        if ((*cursor >= '0') && (*cursor <= '9'))
        {
            reader->cursor = cursor;
            return jx_native_set_error(reader);
        }
    }
    else
    {
        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            has_digit = true;
            parsed = (parsed * 10.0) + (double)(*cursor - '0');
            cursor++;
        }
    }

    if (!has_digit)
    {
        reader->cursor = cursor;
        return jx_native_set_error(reader);
    }

    if (*cursor == '.')
    {
        double scale = 0.1;
        bool has_fraction_digit = false;

        cursor++;
        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            has_fraction_digit = true;
            parsed += (double)(*cursor - '0') * scale;
            scale *= 0.1;
            cursor++;
        }

        if (!has_fraction_digit)
        {
            reader->cursor = cursor;
            return jx_native_set_error(reader);
        }
    }

    if ((*cursor == 'e') || (*cursor == 'E'))
    {
        bool exponent_negative = false;
        bool has_exponent_digit = false;
        bool exponent_overflow = false;
        int exponent = 0;

        cursor++;
        if ((*cursor == '+') || (*cursor == '-'))
        {
            exponent_negative = (*cursor == '-');
            cursor++;
        }

        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            has_exponent_digit = true;
            if (exponent < 308)
            {
                exponent = (exponent * 10) + (*cursor - '0');
            }
            else
            {
                exponent_overflow = true;
            }
            cursor++;
        }

        if ((!has_exponent_digit) || exponent_overflow || (exponent > 308))
        {
            reader->cursor = cursor;
            return jx_native_set_error(reader);
        }

        while (exponent-- > 0)
        {
            parsed = exponent_negative ? (parsed / 10.0) : (parsed * 10.0);
        }
    }

    if (negative)
    {
        parsed = -parsed;
    }

    reader->cursor = cursor;
    *value = parsed;
    return true;
}
#endif

static bool jx_native_parse_unsigned_integer(JX_NATIVE_READER *reader,
                                             uint64_t maximum,
                                             uint64_t *value)
{
    const char *cursor;
    bool has_digit = false;
    uint64_t parsed = 0U;

    if ((reader == NULL) || (value == NULL))
    {
        return false;
    }

    cursor = reader->cursor;
    if (*cursor == '0')
    {
        has_digit = true;
        cursor++;
        if ((*cursor >= '0') && (*cursor <= '9'))
        {
            reader->cursor = cursor;
            return jx_native_set_error(reader);
        }
    }
    else
    {
        while ((*cursor >= '0') && (*cursor <= '9'))
        {
            uint8_t digit = (uint8_t)(*cursor - '0');

            has_digit = true;
            if (parsed > ((maximum - digit) / 10U))
            {
                reader->cursor = cursor;
                return jx_native_set_error(reader);
            }

            parsed = (parsed * 10U) + digit;
            cursor++;
        }
    }

    if (!has_digit)
    {
        reader->cursor = cursor;
        return jx_native_set_error(reader);
    }

    if ((*cursor == '.') || (*cursor == 'e') || (*cursor == 'E'))
    {
        reader->cursor = cursor;
        return jx_native_set_error(reader);
    }

    reader->cursor = cursor;
    *value = parsed;
    return true;
}

static bool jx_native_parse_u32_value(JX_NATIVE_READER *reader, uint32_t *value)
{
    uint64_t parsed;

    if ((reader == NULL) || (value == NULL) || (*reader->cursor == '-'))
    {
        return jx_native_set_error(reader);
    }

    if (!jx_native_parse_unsigned_integer(reader, UINT32_MAX, &parsed))
    {
        return false;
    }

    *value = (uint32_t)parsed;
    return true;
}

static bool jx_native_parse_i32_value(JX_NATIVE_READER *reader, int32_t *value)
{
    bool negative = false;
    uint64_t parsed;
    uint64_t maximum = INT32_MAX;

    if ((reader == NULL) || (value == NULL))
    {
        return false;
    }

    if (*reader->cursor == '-')
    {
        negative = true;
        maximum = (uint64_t)INT32_MAX + 1U;
        reader->cursor++;
    }

    if (!jx_native_parse_unsigned_integer(reader, maximum, &parsed))
    {
        return false;
    }

    if (negative)
    {
        *value = (parsed == maximum) ? INT32_MIN : -(int32_t)parsed;
    }
    else
    {
        *value = (int32_t)parsed;
    }

    return true;
}

static bool jx_native_parse_u64_value(JX_NATIVE_READER *reader, uint64_t *value)
{
    uint64_t parsed;

    if ((reader == NULL) || (value == NULL) || (*reader->cursor == '-'))
    {
        return jx_native_set_error(reader);
    }

    if (!jx_native_parse_unsigned_integer(reader, UINT64_MAX, &parsed))
    {
        return false;
    }

    *value = parsed;
    return true;
}

static bool jx_native_parse_i64_value(JX_NATIVE_READER *reader, int64_t *value)
{
    bool negative = false;
    uint64_t parsed;
    uint64_t maximum = INT64_MAX;

    if ((reader == NULL) || (value == NULL))
    {
        return false;
    }

    if (*reader->cursor == '-')
    {
        negative = true;
        maximum = (uint64_t)INT64_MAX + 1U;
        reader->cursor++;
    }

    if (!jx_native_parse_unsigned_integer(reader, maximum, &parsed))
    {
        return false;
    }

    if (negative)
    {
        *value = (parsed == maximum) ? INT64_MIN : -(int64_t)parsed;
    }
    else
    {
        *value = (int64_t)parsed;
    }

    return true;
}

static bool jx_native_skip_object(JX_NATIVE_READER *reader)
{
    if ((reader == NULL) || (*reader->cursor != '{'))
    {
        return jx_native_set_error(reader);
    }

    if (!jx_native_enter_container(reader))
    {
        return false;
    }

    reader->cursor++;
    jx_native_skip_ws(reader);

    if (*reader->cursor == '}')
    {
        reader->cursor++;
        reader->depth--;
        return true;
    }

    while (*reader->cursor != '\0')
    {
        if (!jx_native_skip_string(reader))
        {
            reader->depth--;
            return false;
        }

        jx_native_skip_ws(reader);
        if (*reader->cursor != ':')
        {
            reader->depth--;
            return jx_native_set_error(reader);
        }

        reader->cursor++;
        if (!jx_native_skip_value(reader))
        {
            reader->depth--;
            return false;
        }

        jx_native_skip_ws(reader);
        if (*reader->cursor == '}')
        {
            reader->cursor++;
            reader->depth--;
            return true;
        }

        if (*reader->cursor != ',')
        {
            reader->depth--;
            return jx_native_set_error(reader);
        }

        reader->cursor++;
        jx_native_skip_ws(reader);
    }

    reader->depth--;
    return jx_native_set_error(reader);
}

static bool jx_native_skip_array(JX_NATIVE_READER *reader)
{
    if ((reader == NULL) || (*reader->cursor != '['))
    {
        return jx_native_set_error(reader);
    }

    if (!jx_native_enter_container(reader))
    {
        return false;
    }

    reader->cursor++;
    jx_native_skip_ws(reader);

    if (*reader->cursor == ']')
    {
        reader->cursor++;
        reader->depth--;
        return true;
    }

    while (*reader->cursor != '\0')
    {
        if (!jx_native_skip_value(reader))
        {
            reader->depth--;
            return false;
        }

        jx_native_skip_ws(reader);
        if (*reader->cursor == ']')
        {
            reader->cursor++;
            reader->depth--;
            return true;
        }

        if (*reader->cursor != ',')
        {
            reader->depth--;
            return jx_native_set_error(reader);
        }

        reader->cursor++;
        jx_native_skip_ws(reader);
    }

    reader->depth--;
    return jx_native_set_error(reader);
}

static bool jx_native_skip_value(JX_NATIVE_READER *reader)
{
    if (reader == NULL)
    {
        return false;
    }

    jx_native_skip_ws(reader);

    switch (*reader->cursor)
    {
    case '{':
        return jx_native_skip_object(reader);

    case '[':
        return jx_native_skip_array(reader);

    case '"':
        return jx_native_skip_string(reader);

    case 't':
        return jx_native_match_literal(reader, "true");

    case 'f':
        return jx_native_match_literal(reader, "false");

    case 'n':
        return jx_native_match_literal(reader, "null");

    default:
        if ((*reader->cursor == '-') || ((*reader->cursor >= '0') && (*reader->cursor <= '9')))
        {
            return jx_native_skip_number(reader);
        }
        return jx_native_set_error(reader);
    }
}

static JX_ELEMENT *jx_native_find_element(JX_ELEMENT *elements, size_t element_count, const char *property)
{
    if ((elements == NULL) || (property == NULL))
    {
        return NULL;
    }

    for (size_t i = 0U; i < element_count; ++i)
    {
        if (strncmp(elements[i].property, property, JX_PROPERTY_MAX_SIZE) == 0)
        {
            return &elements[i];
        }
    }

    return NULL;
}

static JX_STATUS jx_native_handle_type_mismatch(JX_ELEMENT *element, JX_PARSE_MODE mode)
{
    if ((element == NULL) || (mode == JX_MODE_STRICT))
    {
        return JX_ERROR;
    }

    jx_clear_status(element);
    return JX_SUCCESS;
}

static JX_STATUS jx_native_parse_element_value(JX_NATIVE_READER *reader, JX_ELEMENT *element, JX_PARSE_MODE mode)
{
    if ((reader == NULL) || (element == NULL))
    {
        return JX_ERROR;
    }

    jx_native_skip_ws(reader);

    switch (element->type)
    {
    case JX_NULL:
        if (strncmp(reader->cursor, "null", 4U) == 0)
        {
            reader->cursor += 4U;
            jx_set_updated(element);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_BOOLEAN:
        if (strncmp(reader->cursor, "true", 4U) == 0)
        {
            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }
            reader->cursor += 4U;
            jx_set_bool(element, true);
            return JX_SUCCESS;
        }
        if (strncmp(reader->cursor, "false", 5U) == 0)
        {
            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }
            reader->cursor += 5U;
            jx_set_bool(element, false);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_NUMBER:
#if JX_ENABLE_DOUBLE
        if ((*reader->cursor == '-') || ((*reader->cursor >= '0') && (*reader->cursor <= '9')))
        {
            double value;

            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }
            if (!jx_native_parse_number_value(reader, &value))
            {
                return JX_ERROR;
            }
            jx_set_number(element, value);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
#else
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, JX_MODE_STRICT);
        }
#endif
        return JX_ERROR;

    case JX_U32:
        if ((*reader->cursor == '-') || ((*reader->cursor >= '0') && (*reader->cursor <= '9')))
        {
            uint32_t value;

            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }
            if (!jx_native_parse_u32_value(reader, &value))
            {
                return JX_ERROR;
            }
            jx_set_u32(element, value);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_I32:
        if ((*reader->cursor == '-') || ((*reader->cursor >= '0') && (*reader->cursor <= '9')))
        {
            int32_t value;

            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }
            if (!jx_native_parse_i32_value(reader, &value))
            {
                return JX_ERROR;
            }
            jx_set_i32(element, value);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_U64:
        if ((*reader->cursor == '-') || ((*reader->cursor >= '0') && (*reader->cursor <= '9')))
        {
            uint64_t value;

            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }
            if (!jx_native_parse_u64_value(reader, &value))
            {
                return JX_ERROR;
            }
            jx_set_u64(element, value);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_I64:
        if ((*reader->cursor == '-') || ((*reader->cursor >= '0') && (*reader->cursor <= '9')))
        {
            int64_t value;

            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }
            if (!jx_native_parse_i64_value(reader, &value))
            {
                return JX_ERROR;
            }
            jx_set_i64(element, value);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_STRING:
        if (*reader->cursor == '"')
        {
            size_t capacity;

            if (element->value_p == NULL)
            {
                return JX_ERROR;
            }

            capacity = (element->value_capacity != 0U) ? element->value_capacity : JX_PROPERTY_MAX_SIZE;
            if (!jx_native_parse_string_into_buffer(reader, (char *)(uintptr_t)element->value_p, capacity))
            {
                return JX_ERROR;
            }
            jx_set_updated(element);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_OBJECT:
        if (*reader->cursor == '{')
        {
            if ((element->element != NULL) &&
                (jx_native_parse_object_into_elements(reader, element->element,
                                                      element->value_len, mode) != JX_SUCCESS))
            {
                return JX_ERROR;
            }

            if ((element->element == NULL) && !jx_native_skip_object(reader))
            {
                return JX_ERROR;
            }

            jx_set_updated(element);
            return JX_SUCCESS;
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    case JX_ARRAY:
        if (*reader->cursor == '[')
        {
            return jx_native_parse_array_into_elements(reader, element, mode);
        }
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;

    default:
        if (jx_native_skip_value(reader))
        {
            return jx_native_handle_type_mismatch(element, mode);
        }
        return JX_ERROR;
    }
}

static JX_STATUS jx_native_parse_array_into_elements(JX_NATIVE_READER *reader, JX_ELEMENT *element, JX_PARSE_MODE mode)
{
    uint8_t capacity;
    uint8_t parsed_count = 0U;

    if ((reader == NULL) || (element == NULL) || (*reader->cursor != '['))
    {
        return JX_ERROR;
    }

    capacity = (element->value_capacity != 0U) ? element->value_capacity : element->value_len;
    if ((capacity > 0U) && (element->element == NULL))
    {
        return JX_ERROR;
    }

    if (!jx_native_enter_container(reader))
    {
        return JX_ERROR;
    }

    reader->cursor++;
    jx_native_skip_ws(reader);

    if (*reader->cursor == ']')
    {
        reader->cursor++;
        reader->depth--;
        element->value_len = 0U;
        jx_set_updated(element);
        return JX_SUCCESS;
    }

    while (*reader->cursor != '\0')
    {
        JX_ELEMENT *item;

        if (parsed_count >= capacity)
        {
            reader->depth--;
            return JX_ERROR;
        }

        item = &element->element[parsed_count];
        if (jx_native_parse_element_value(reader, item, mode) != JX_SUCCESS)
        {
            reader->depth--;
            return JX_ERROR;
        }

        jx_set_updated(item);
        parsed_count++;

        jx_native_skip_ws(reader);
        if (*reader->cursor == ']')
        {
            reader->cursor++;
            reader->depth--;
            element->value_len = parsed_count;
            jx_set_updated(element);
            return JX_SUCCESS;
        }

        if (*reader->cursor != ',')
        {
            reader->depth--;
            jx_native_set_error(reader);
            return JX_ERROR;
        }

        reader->cursor++;
        jx_native_skip_ws(reader);
    }

    reader->depth--;
    jx_native_set_error(reader);
    return JX_ERROR;
}

static JX_STATUS jx_native_parse_object_into_elements(JX_NATIVE_READER *reader,
                                                      JX_ELEMENT *elements,
                                                      size_t element_count,
                                                      JX_PARSE_MODE mode)
{
    if ((reader == NULL) || (elements == NULL) || (element_count == 0U) || (*reader->cursor != '{'))
    {
        return JX_ERROR;
    }

    if (!jx_native_enter_container(reader))
    {
        return JX_ERROR;
    }

    reader->cursor++;
    jx_native_skip_ws(reader);

    if (*reader->cursor == '}')
    {
        reader->cursor++;
        reader->depth--;
        goto check_required;
    }

    while (*reader->cursor != '\0')
    {
        char property[JX_PROPERTY_MAX_SIZE];
        JX_ELEMENT *element;

        if (!jx_native_parse_string_into_buffer(reader, property, sizeof(property)))
        {
            reader->depth--;
            return JX_ERROR;
        }

        jx_native_skip_ws(reader);
        if (*reader->cursor != ':')
        {
            reader->depth--;
            jx_native_set_error(reader);
            return JX_ERROR;
        }

        reader->cursor++;
        element = jx_native_find_element(elements, element_count, property);
        if (element == NULL)
        {
            if (!jx_native_skip_value(reader))
            {
                reader->depth--;
                return JX_ERROR;
            }
        }
        else if (jx_native_parse_element_value(reader, element, mode) != JX_SUCCESS)
        {
            reader->depth--;
            return JX_ERROR;
        }

        jx_native_skip_ws(reader);
        if (*reader->cursor == '}')
        {
            reader->cursor++;
            reader->depth--;
            goto check_required;
        }

        if (*reader->cursor != ',')
        {
            reader->depth--;
            jx_native_set_error(reader);
            return JX_ERROR;
        }

        reader->cursor++;
        jx_native_skip_ws(reader);
    }

    reader->depth--;
    jx_native_set_error(reader);
    return JX_ERROR;

check_required:
    if (mode == JX_MODE_STRICT)
    {
        for (size_t i = 0U; i < element_count; ++i)
        {
            if (!jx_is_updated(&elements[i]))
            {
                return JX_ERROR;
            }
        }
    }

    return JX_SUCCESS;
}

static void jx_native_writer_putc(JX_NATIVE_WRITER *writer, char c)
{
    if ((writer == NULL) || writer->failed)
    {
        return;
    }

    if ((writer->pos + 1U) >= writer->size)
    {
        writer->failed = true;
        return;
    }

    writer->buffer[writer->pos++] = c;
    writer->buffer[writer->pos] = '\0';
}

static void jx_native_writer_puts(JX_NATIVE_WRITER *writer, const char *text)
{
    if (text == NULL)
    {
        return;
    }

    while (*text != '\0')
    {
        jx_native_writer_putc(writer, *text++);
    }
}

static void jx_native_writer_indent(JX_NATIVE_WRITER *writer, uint8_t depth)
{
    if (!writer->formatted)
    {
        return;
    }

    jx_native_writer_putc(writer, '\n');
    for (uint8_t i = 0U; i < depth; ++i)
    {
        jx_native_writer_puts(writer, "\t");
    }
}

static bool jx_native_print_string(JX_NATIVE_WRITER *writer, const char *value)
{
    jx_native_writer_putc(writer, '"');
    if (value != NULL)
    {
        while (*value != '\0')
        {
            switch (*value)
            {
            case '"':
                jx_native_writer_puts(writer, "\\\"");
                break;
            case '\\':
                jx_native_writer_puts(writer, "\\\\");
                break;
            case '\b':
                jx_native_writer_puts(writer, "\\b");
                break;
            case '\f':
                jx_native_writer_puts(writer, "\\f");
                break;
            case '\n':
                jx_native_writer_puts(writer, "\\n");
                break;
            case '\r':
                jx_native_writer_puts(writer, "\\r");
                break;
            case '\t':
                jx_native_writer_puts(writer, "\\t");
                break;
            default:
                if ((unsigned char)*value < 0x20U)
                {
                    writer->failed = true;
                    return false;
                }
                jx_native_writer_putc(writer, *value);
                break;
            }
            value++;
        }
    }
    jx_native_writer_putc(writer, '"');
    return !writer->failed;
}

static void jx_native_print_unsigned(JX_NATIVE_WRITER *writer, uint64_t value)
{
    char digits[20];
    size_t count = 0U;

    do
    {
        digits[count++] = (char)('0' + (value % 10U));
        value /= 10U;
    } while ((value != 0U) && (count < sizeof(digits)));

    while (count > 0U)
    {
        jx_native_writer_putc(writer, digits[--count]);
    }
}

static void jx_native_print_signed(JX_NATIVE_WRITER *writer, int64_t value)
{
    uint64_t magnitude;

    if (value < 0)
    {
        jx_native_writer_putc(writer, '-');
        magnitude = (uint64_t)(-(value + 1)) + 1U;
    }
    else
    {
        magnitude = (uint64_t)value;
    }

    jx_native_print_unsigned(writer, magnitude);
}

#if JX_ENABLE_DOUBLE
static bool jx_native_print_number(JX_NATIVE_WRITER *writer, double value)
{
    uint64_t integer_part;
    double fraction;

    if (value < 0.0)
    {
        jx_native_writer_putc(writer, '-');
        value = -value;
    }

    if (value > 18446744073709551615.0)
    {
        return false;
    }

    integer_part = (uint64_t)value;
    fraction = value - (double)integer_part;
    jx_native_print_unsigned(writer, integer_part);

    if (fraction > 0.0000005)
    {
        char fractional_digits[6];
        int last_non_zero = -1;

        jx_native_writer_putc(writer, '.');
        for (uint8_t i = 0U; i < sizeof(fractional_digits); ++i)
        {
            int digit;

            fraction *= 10.0;
            digit = (int)fraction;
            if (digit < 0)
            {
                digit = 0;
            }
            if (digit > 9)
            {
                digit = 9;
            }
            fractional_digits[i] = (char)('0' + digit);
            if (digit != 0)
            {
                last_non_zero = (int)i;
            }
            fraction -= (double)digit;
        }

        if (last_non_zero < 0)
        {
            jx_native_writer_putc(writer, '0');
        }
        else
        {
            for (int i = 0; i <= last_non_zero; ++i)
            {
                jx_native_writer_putc(writer, fractional_digits[i]);
            }
        }
    }

    return !writer->failed;
}
#endif

static bool jx_native_write_elements(JX_NATIVE_WRITER *writer,
                                     JX_ELEMENT *elements,
                                     size_t element_count,
                                     uint8_t depth,
                                     bool object_context)
{
    bool first = true;
    char open_char = object_context ? '{' : '[';
    char close_char = object_context ? '}' : ']';

    if ((writer == NULL) || ((elements == NULL) && (element_count != 0U)))
    {
        return false;
    }

    jx_native_writer_putc(writer, open_char);
    for (size_t i = 0U; i < element_count; ++i)
    {
        if (!first)
        {
            jx_native_writer_putc(writer, ',');
        }
        first = false;

        if (writer->formatted)
        {
            jx_native_writer_indent(writer, (uint8_t)(depth + 1U));
        }

        if (object_context)
        {
            if (!jx_native_print_string(writer, elements[i].property))
            {
                return false;
            }
            jx_native_writer_putc(writer, ':');
            if (writer->formatted)
            {
                jx_native_writer_putc(writer, '\t');
            }
        }

        if (!jx_native_write_element_value(writer, &elements[i], (uint8_t)(depth + 1U)))
        {
            return false;
        }
    }

    if ((element_count != 0U) && writer->formatted)
    {
        jx_native_writer_indent(writer, depth);
    }
    jx_native_writer_putc(writer, close_char);
    return !writer->failed;
}

static bool jx_native_write_element_value(JX_NATIVE_WRITER *writer, JX_ELEMENT *element, uint8_t depth)
{
    if ((writer == NULL) || (element == NULL))
    {
        return false;
    }

    switch (element->type)
    {
    case JX_NULL:
        jx_native_writer_puts(writer, "null");
        break;

    case JX_BOOLEAN:
        if (element->value_p == NULL)
        {
            return false;
        }
        jx_native_writer_puts(writer, *((bool *)element->value_p) ? "true" : "false");
        break;

    case JX_NUMBER:
#if JX_ENABLE_DOUBLE
        if (element->value_p == NULL)
        {
            return false;
        }
        return jx_native_print_number(writer, *((double *)element->value_p));
#else
        return false;
#endif

    case JX_U32:
        if (element->value_p == NULL)
        {
            return false;
        }
        jx_native_print_unsigned(writer, *((uint32_t *)element->value_p));
        break;

    case JX_I32:
        if (element->value_p == NULL)
        {
            return false;
        }
        jx_native_print_signed(writer, *((int32_t *)element->value_p));
        break;

    case JX_U64:
        if (element->value_p == NULL)
        {
            return false;
        }
        jx_native_print_unsigned(writer, *((uint64_t *)element->value_p));
        break;

    case JX_I64:
        if (element->value_p == NULL)
        {
            return false;
        }
        jx_native_print_signed(writer, *((int64_t *)element->value_p));
        break;

    case JX_STRING:
        if (element->value_p == NULL)
        {
            return false;
        }
        return jx_native_print_string(writer, (const char *)element->value_p);

    case JX_OBJECT:
        return jx_native_write_elements(writer,
                                        element->element,
                                        element->element == NULL ? 0U : (size_t)element->value_len,
                                        depth,
                                        true);

    case JX_ARRAY:
        if ((element->element == NULL) && (element->value_len != 0U))
        {
            return false;
        }
        return jx_native_write_elements(writer,
                                        element->element,
                                        element->element == NULL ? 0U : (size_t)element->value_len,
                                        depth,
                                        false);

    default:
        return false;
    }

    return !writer->failed;
}

void jx_backend_init_hooks(void *(*malloc_fn)(size_t size), void (*free_fn)(void *ptr))
{
    (void)malloc_fn;
    (void)free_fn;
    jx_native_error_ptr = NULL;
}

void jx_backend_reset_hooks(void)
{
    jx_native_error_ptr = NULL;
}

const char *jx_backend_get_error_ptr(void)
{
    return jx_native_error_ptr;
}

JX_STATUS jx_backend_parse_into_elements(char *buffer,
                                         JX_ELEMENT *elements,
                                         size_t element_count,
                                         JX_PARSE_MODE mode)
{
    JX_NATIVE_READER reader;

    if ((buffer == NULL) || (elements == NULL) || (element_count == 0U) ||
        ((mode != JX_MODE_RELAXED) && (mode != JX_MODE_STRICT)))
    {
        return JX_ERROR;
    }

    reader.start = buffer;
    reader.cursor = buffer;
    reader.error = NULL;
    reader.depth = 0U;
    jx_native_error_ptr = NULL;

    for (size_t i = 0U; i < element_count; ++i)
    {
        jx_clear_status(&elements[i]);
    }

    jx_native_skip_ws(&reader);
    if (jx_native_parse_object_into_elements(&reader, elements, element_count, mode) != JX_SUCCESS)
    {
        jx_native_error_ptr = (reader.error != NULL) ? reader.error : reader.cursor;
        return JX_ERROR;
    }

    jx_native_skip_ws(&reader);
    if (*reader.cursor != '\0')
    {
        jx_native_error_ptr = reader.cursor;
        return JX_ERROR;
    }

    return JX_SUCCESS;
}


bool jx_backend_write_elements(JX_ELEMENT *elements,
                               size_t element_count,
                               char *buffer,
                               size_t buffer_size,
                               JX_FORMAT format)
{
    JX_NATIVE_WRITER writer;

    if ((elements == NULL) || (element_count == 0U) ||
        (buffer == NULL) || (buffer_size == 0U) ||
        ((format != JX_MINIFIED) && (format != JX_FORMATTED)))
    {
        return false;
    }

    memset(&writer, 0, sizeof(writer));
    writer.buffer = buffer;
    writer.size = buffer_size;
    writer.formatted = (format == JX_FORMATTED);
    writer.buffer[0] = '\0';

    if (!jx_native_write_elements(&writer, elements, element_count, 0U, true))
    {
        return false;
    }

    return !writer.failed;
}
