/**************************************************************************/
/*                                                                        */
/*  @file jx_parser.c                                                     */
/*  @brief Core Parsing and Serialization Engine for JsonX                */
/*                                                                        */
/*  This file contains the main implementation for JSON <-> C structure   */
/*  conversion, using the JX_ELEMENT[] model.                             */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#include "jx_api.h"
#include "jx_internal.h"
#ifdef JX_USE_FREERTOS
#include "FreeRTOS.h"
#endif

#ifndef ALIGN_4
#define ALIGN_4(x)  (((x) + 3) & ~3)
#endif

/**************************************************************************/
/*                                                                        */
/*  Internal Parser Instance                                              */
/*                                                                        */
/**************************************************************************/

static JX_PARSER *JSON_Parser = NULL;

static cJSON *_jx_json_to_object(char *buffer);
static char *_jx_object_to_json(cJSON *object, char *buffer, const int buffer_size, JX_FORMAT format);
static JX_STATUS _jx_object_to_struct(cJSON *main_object, JX_ELEMENT *element, size_t size, JX_PARSE_MODE mode);
static JX_STATUS _jx_struct_to_object(cJSON *object, JX_ELEMENT *element, size_t size);


/**************************************************************************/
/*                                                                        */
/*  Initialization / Deinitialization                                     */
/*                                                                        */
/**************************************************************************/


#ifdef JX_USE_THREADX
JX_STATUS jx_init(TX_BYTE_POOL *byte_pool)
{
    if ((!byte_pool) || JSON_Parser)
    {
        return JX_ERROR;
    }

    /* Allocate memory for parser instance */
    tx_byte_allocate(byte_pool, (VOID**)&JSON_Parser, ALIGN_4(sizeof(JX_PARSER)), TX_NO_WAIT);
	if (!JSON_Parser)
		return JX_ERROR;

	memset(JSON_Parser, 0, sizeof(JX_PARSER));
    JSON_Parser->byte_pool         = byte_pool;
    JSON_Parser->hooks.malloc_fn   = jx_alloc_memory;
    JSON_Parser->hooks.free_fn     = jx_free_memory;

    cJSON_Hooks cJSON_hooks = { JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn };
    cJSON_InitHooks(&cJSON_hooks);

    JSON_Parser->state = JX_INITIALIZED;
#ifdef JX_DEBUG
    jx_log("JSON Parser Initialized\r\n");
#endif
    return JX_SUCCESS;
}
#endif

#if  defined(JX_USE_FREERTOS) || defined(JX_USE_HEAP_BAREMETAL)
JX_STATUS jx_init(void)
{
	if (JSON_Parser)
	{
		return JX_ERROR;
	}

	/* Allocate memory for parser instance */
	JSON_Parser = jx_alloc_memory(sizeof(JX_PARSER));
	if (!JSON_Parser)
		return JX_ERROR;

	memset(JSON_Parser, 0, sizeof(JX_PARSER));
	JSON_Parser->hooks.malloc_fn   = jx_alloc_memory;
	JSON_Parser->hooks.free_fn     = jx_free_memory;
	cJSON_Hooks cJSON_hooks = { JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn };
	cJSON_InitHooks(&cJSON_hooks);

	JSON_Parser->state = JX_INITIALIZED;
#ifdef JX_DEBUG
	jx_log("JSON Parser Initialized\r\n");
#endif
	return JX_SUCCESS;
}
#endif

#ifdef JX_USE_BAREMETAL
#ifndef JX_USE_HEAP_BAREMETAL
JX_STATUS jx_init(void *buffer, size_t size)
{
	 if (!buffer || size <  ALIGN_4(sizeof(JX_PARSER)))
	 {
	        return JX_ERROR;
	 }
	JSON_Parser = (JX_PARSER*)buffer;
	buffer += ALIGN_4(sizeof(JX_PARSER));
	size -= ALIGN_4(sizeof(JX_PARSER));
	memset(JSON_Parser, 0, sizeof(JX_PARSER));
#ifndef JX_USE_HEAP_BAREMETAL
	JSON_Parser->allocator = jx_static_allocator_init(buffer, size);
	if(!JSON_Parser->allocator)
	{
		return JX_ERROR;
	}
#endif
	JSON_Parser->hooks.malloc_fn   = jx_static_malloc;
	JSON_Parser->hooks.free_fn     = jx_static_free;
	JSON_Parser->hooks.reset_fn    = jx_static_reset;
	cJSON_Hooks cJSON_hooks = { JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn };
	cJSON_InitHooks(&cJSON_hooks);

	JSON_Parser->state = JX_INITIALIZED;
#ifdef JX_DEBUG
	jx_log("JSON Parser Initialized\r\n");
#endif
	return JX_SUCCESS;
}
#endif

#elif defined(JX_USE_CUSTOM_ALLOCATOR)
JX_STATUS jx_init(JX_HOOKS *hooks)
{
	if (!hooks || (!hooks->malloc_fn) || (!hooks->free_fn) || JSON_Parser)
	{
		return JX_ERROR;
	}

	/* Allocate memory for parser instance */
	JSON_Parser = hooks->malloc_fn(ALIGN_4(sizeof(JX_PARSER)));
	if (!JSON_Parser)
		return JX_ERROR;

	memset(JSON_Parser, 0, sizeof(JX_PARSER));
	JSON_Parser->hooks.malloc_fn   = hooks->malloc_fn;
	JSON_Parser->hooks.free_fn     = hooks->free_fn;
	cJSON_Hooks cJSON_hooks = { JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn };
	cJSON_InitHooks(&cJSON_hooks);

	JSON_Parser->state = JX_INITIALIZED;
#ifdef JX_DEBUG
	jx_log("JSON Parser Initialized\r\n");
#endif
	return JX_SUCCESS;
}
#endif

void jx_parser_deinit(void)
{
    if (!JSON_Parser)
    {
        return;
    }

    cJSON_Hooks cJSON_hooks = { NULL, NULL };
    cJSON_InitHooks(&cJSON_hooks);
    memset(JSON_Parser, 0, sizeof(JX_PARSER));
#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
    // do not call jx_free_memory
#else
    jx_free_memory((void*)JSON_Parser);
#endif
    JSON_Parser = NULL;

#ifdef JX_DEBUG
    jx_log("JX Parser deinitialized\r\n");
#endif
}


/**************************************************************************/
/*                                                                        */
/*  Memory Management                                                     */
/*                                                                        */
/**************************************************************************/

void *jx_alloc_memory(size_t memory_size)
{
    void *memory_ptr = NULL;

#ifdef JX_USE_THREADX
    tx_byte_allocate(JSON_Parser->byte_pool, &memory_ptr, ALIGN_4(memory_size), TX_NO_WAIT);
#endif
#ifdef JX_USE_FREERTOS
    memory_ptr = pvPortMalloc(ALIGN_4(memory_size));
#endif
#ifdef JX_USE_BAREMETAL
#ifdef JX_USE_HEAP_BAREMETAL
    memory_ptr = malloc(ALIGN_4(memory_size));
#else
    memory_ptr = jx_static_malloc(ALIGN_4(memory_size));
#endif
#endif
#ifdef JX_USE_CUSTOM_ALLOCATOR
    if((JSON_Parser == NULL) || (JSON_Parser->hooks.malloc_fn == NULL))
		return NULL;
    memory_ptr = JSON_Parser->hooks.malloc_fn(ALIGN_4(memory_size));
#endif
    return memory_ptr;
}

void jx_free_memory(void *memory_ptr)
{
#ifdef JX_USE_THREADX
    tx_byte_release(memory_ptr);
#endif
#ifdef JX_USE_FREERTOS
    vPortFree(memory_ptr);
#endif
#ifdef JX_USE_BAREMETAL
#ifdef JX_USE_HEAP_BAREMETAL
    free(memory_ptr);
#else
    jx_static_free(memory_ptr);
#endif
#endif
#ifdef JX_USE_CUSTOM_ALLOCATOR
    if((JSON_Parser == NULL) || (JSON_Parser->hooks.free_fn == NULL))
    	return;
    JSON_Parser->hooks.free_fn(memory_ptr);
#endif
}

/**************************************************************************/
/*                                                                        */
/*  High-Level Interface                                                  */
/*                                                                        */
/**************************************************************************/

JX_STATUS jx_struct_to_json(JX_ELEMENT *element, size_t element_size, char *buffer, size_t buffer_size, JX_FORMAT format)
{
#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
	jx_static_reset();
#endif
    cJSON *object = cJSON_CreateObject();
    if (!object)
        return JX_ERROR;

    if (_jx_struct_to_object(object, element, element_size) != JX_SUCCESS)
    {
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
        cJSON_Delete(object);
#endif
        return JX_ERROR;
    }

    if (!_jx_object_to_json(object, buffer, buffer_size, format))
    {
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
        cJSON_Delete(object);
#endif
        return JX_ERROR;
    }
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
    cJSON_Delete(object);
#endif
    return JX_SUCCESS;
}

JX_STATUS jx_json_to_struct(char *buffer, JX_ELEMENT *element, size_t element_size, JX_PARSE_MODE mode)
{
#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
	jx_static_reset();
#endif
    cJSON *object = _jx_json_to_object(buffer);
    if (!object)
        return JX_ERROR;

    JX_STATUS result = _jx_object_to_struct(object, element, element_size, mode);
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
    cJSON_Delete(object);
#endif
    return result;
}

/**************************************************************************/
/*                                                                        */
/*  Internal Conversion: Struct <-> cJSON Object                                   */
/*                                                                        */
/**************************************************************************/

static JX_STATUS _jx_struct_to_object(cJSON *object, JX_ELEMENT *element, size_t size)
{
    cJSON *node = NULL;
    size_t i;

    if (!object || size == 0)
    {
        return JX_ERROR;
    }

    for (i = 0; i < size; ++i)
    {
        const char *prop = element[i].property;

        switch (element[i].type)
        {
        case JX_STRING:
            node = cJSON_CreateString((const char *)element[i].value_p);
            break;

        case JX_BOOLEAN:
            node = cJSON_CreateBool(*((bool *)element[i].value_p));
            break;

        case JX_NUMBER:
            node = cJSON_CreateNumber(*((double *)element[i].value_p));
            break;

        case JX_OBJECT:
            node = cJSON_CreateObject();
            if (_jx_struct_to_object(node, element[i].element, element[i].value_len) != JX_SUCCESS)
                return JX_ERROR;
            break;

        case JX_ARRAY:
            node = cJSON_CreateArray();
            if (_jx_struct_to_object(node, element[i].element, element[i].value_len) != JX_SUCCESS)
                return JX_ERROR;
            break;

        default:
            break;
        }

        if (node)
        {
            bool added = (object->type == cJSON_Array)
                         ? cJSON_AddItemToArray(object, node)
                         : cJSON_AddItemToObject(object, prop, node);

            if (!added)
            {
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
                cJSON_Delete(node);
#endif
            }
        }
    }

    return JX_SUCCESS;
}


static JX_STATUS _jx_object_to_struct(cJSON *main_object, JX_ELEMENT *element, size_t size, JX_PARSE_MODE mode)
{
    IF_JX_ERROR_EXIT(main_object);

    for (size_t i = 0; i < size; ++i)
    {
        cJSON *object = NULL;

        JX_RETURN_IF_NULL(&element[i]);
        JX_RETURN_IF_NULL(element[i].property);

        if (element[i].property[0] != 0)
        {
            object = cJSON_GetObjectItemCaseSensitive(main_object, element[i].property);
        }
        else
        {
            object = main_object;
        }

        if (!object)
        {
            if (mode == JX_MODE_STRICT)
                goto end;
            else
                continue;
        }

        switch (element[i].type)
        {
        case JX_NULL:
            if (cJSON_IsNull(object))
                jx_set_null_u32(&element[i]);
            else
                jx_clear_status(&element[i]);
            break;

        case JX_BOOLEAN:
            if (cJSON_IsBool(object))
                jx_set_bool(&element[i], cJSON_IsTrue(object));
            else
                jx_clear_status(&element[i]);
            break;

        case JX_NUMBER:
            if (cJSON_IsNumber(object))
                jx_set_number(&element[i], object->valuedouble);
            else
                jx_clear_status(&element[i]);
            break;

        case JX_STRING:
            if (cJSON_IsString(object) && object->valuestring)
                jx_set_string(&element[i], object->valuestring);
            else
                jx_clear_status(&element[i]);
            break;

        case JX_OBJECT:
            if (cJSON_IsObject(object))
            {
                if (_jx_object_to_struct(object, element[i].element, element[i].value_len, mode) != JX_SUCCESS)
                    goto end;
                jx_set_updated(&element[i]);
            }
            else
            {
                jx_clear_status(&element[i]);
            }
            break;

        case JX_ARRAY:
            element[i].value_len = cJSON_GetArraySize(object);
            for (size_t j = 0; j < element[i].value_len; ++j)
            {
                cJSON *item = cJSON_GetArrayItem(object, j);
                if (!item || !element[i].element)
                    continue;

                size_t len = element[i].element[j].value_len;
                if (_jx_object_to_struct(item, &element[i].element[j], len ? len : 1, mode) != JX_SUCCESS)
                    goto end;

                jx_set_updated(&element[i].element[j]);
            }
            jx_set_updated(&element[i]);
            break;

        default:
            break;
        }
    }

    return JX_SUCCESS;

end:
    return JX_ERROR;
}


/**************************************************************************/
/*                                                                        */
/*  Internal Conversion: JSON string <-> cJSON Object                              */
/*                                                                        */
/**************************************************************************/

static cJSON *_jx_json_to_object(char *buffer)
{
    return cJSON_Parse(buffer);
}

static char *_jx_object_to_json(cJSON *object, char *buffer, const int buffer_size, JX_FORMAT format)
{
    if (cJSON_PrintPreallocated(object, buffer, buffer_size, (format != JX_MINIFIED)))
        return buffer;
    return NULL;
}


