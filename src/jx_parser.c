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
#include "../private/jx_backend.h"
#include "../private/jx_internal.h"
#include <limits.h>
#if defined(JX_USE_BAREMETAL) && defined(JX_USE_HEAP_BAREMETAL)
#include <stdlib.h>
#endif
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

static bool _jx_is_initialized(void);


/**************************************************************************/
/*                                                                        */
/*  Initialization / Deinitialization                                     */
/*                                                                        */
/**************************************************************************/


#ifdef JX_USE_THREADX
JX_STATUS jx_init(TX_BYTE_POOL *byte_pool)
{
    UINT tx_status;
    JX_PARSER *parser = NULL;

    if ((!byte_pool) || JSON_Parser)
    {
        return JX_ERROR;
    }

    tx_status = tx_byte_allocate(byte_pool, (VOID **)&parser,
                                 ALIGN_4(sizeof(JX_PARSER)), TX_NO_WAIT);
    if ((tx_status != TX_SUCCESS) || (parser == NULL))
    {
        return JX_ERROR;
    }

    JSON_Parser = parser;
    memset(JSON_Parser, 0, sizeof(JX_PARSER));
    JSON_Parser->byte_pool         = byte_pool;
    JSON_Parser->hooks.malloc_fn   = jx_alloc_memory;
    JSON_Parser->hooks.free_fn     = jx_free_memory;

    jx_backend_init_hooks(JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn);

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
	jx_backend_init_hooks(JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn);

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
	uint8_t *pool_cursor;
	size_t parser_size = ALIGN_4(sizeof(JX_PARSER));

	if(!buffer || JSON_Parser || size < parser_size)
	{
		return JX_ERROR;
	}

	pool_cursor = (uint8_t *)buffer;
	JSON_Parser = (JX_PARSER *)pool_cursor;
	pool_cursor += parser_size;
	size -= parser_size;
	memset(JSON_Parser, 0, sizeof(JX_PARSER));
	JSON_Parser->allocator = jx_static_allocator_init(pool_cursor, size);
	if(!JSON_Parser->allocator)
	{
		memset(JSON_Parser, 0, sizeof(JX_PARSER));
		JSON_Parser = NULL;
		return JX_ERROR;
	}
	JSON_Parser->hooks.malloc_fn   = jx_static_malloc;
	JSON_Parser->hooks.free_fn     = jx_static_free;
	JSON_Parser->hooks.reset_fn    = jx_static_reset;
	jx_backend_init_hooks(JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn);

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
	jx_backend_init_hooks(JSON_Parser->hooks.malloc_fn, JSON_Parser->hooks.free_fn);

	JSON_Parser->state = JX_INITIALIZED;
#ifdef JX_DEBUG
	jx_log("JSON Parser Initialized\r\n");
#endif
	return JX_SUCCESS;
}
#endif

void jx_parser_deinit(void)
{
    JX_PARSER *parser = JSON_Parser;

    if (!JSON_Parser)
    {
        return;
    }

    jx_backend_reset_hooks();
    JSON_Parser = NULL;
#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
    memset(parser, 0, sizeof(JX_PARSER));
#elif defined(JX_USE_CUSTOM_ALLOCATOR)
    void (*free_fn)(void *) = parser->hooks.free_fn;
    memset(parser, 0, sizeof(JX_PARSER));
    if (free_fn)
    {
        free_fn((void *)parser);
    }
#elif defined(JX_USE_THREADX)
    memset(parser, 0, sizeof(JX_PARSER));
    tx_byte_release((void *)parser);
#elif defined(JX_USE_FREERTOS)
    memset(parser, 0, sizeof(JX_PARSER));
    vPortFree((void *)parser);
#elif defined(JX_USE_HEAP_BAREMETAL)
    memset(parser, 0, sizeof(JX_PARSER));
    free((void *)parser);
#else
    memset(parser, 0, sizeof(JX_PARSER));
#endif

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

    if (memory_size == 0U)
    {
        return NULL;
    }

#ifdef JX_USE_THREADX
    if ((JSON_Parser == NULL) || (JSON_Parser->byte_pool == NULL))
    {
        return NULL;
    }
    if (tx_byte_allocate(JSON_Parser->byte_pool, &memory_ptr,
                         ALIGN_4(memory_size), TX_NO_WAIT) != TX_SUCCESS)
    {
        return NULL;
    }
#endif
#ifdef JX_USE_FREERTOS
    memory_ptr = pvPortMalloc(ALIGN_4(memory_size));
#endif
#ifdef JX_USE_BAREMETAL
#ifdef JX_USE_HEAP_BAREMETAL
    memory_ptr = malloc(ALIGN_4(memory_size));
#else
    if (JSON_Parser == NULL)
    {
        return NULL;
    }
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
    if (memory_ptr == NULL)
    {
        return;
    }

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
    if ((!_jx_is_initialized()) || (!element) || (element_size == 0U) ||
        (!buffer) || (buffer_size == 0U) || (buffer_size > (size_t)INT_MAX) ||
        ((format != JX_MINIFIED) && (format != JX_FORMATTED)))
    {
        return JX_ERROR;
    }

#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
	jx_static_reset();
#endif
    if (!jx_backend_write_elements(element, element_size, buffer, buffer_size, format))
    {
        return JX_ERROR;
    }

    return JX_SUCCESS;
}

JX_STATUS jx_json_to_struct(char *buffer, JX_ELEMENT *element, size_t element_size, JX_PARSE_MODE mode)
{
    if ((!_jx_is_initialized()) || (!buffer) || (!element) || (element_size == 0U) ||
        ((mode != JX_MODE_RELAXED) && (mode != JX_MODE_STRICT)))
    {
        return JX_ERROR;
    }

#if defined(JX_USE_BAREMETAL) && !defined(JX_USE_HEAP_BAREMETAL)
	jx_static_reset();
#endif
    return jx_backend_parse_into_elements(buffer, element, element_size, mode);
}

size_t jx_get_last_error_offset(const char *buffer)
{
    const char *error_ptr = jx_backend_get_error_ptr();

    if ((buffer == NULL) || (error_ptr == NULL) || (error_ptr < buffer))
    {
        return (size_t)-1;
    }

    return (size_t)(error_ptr - buffer);
}

static bool _jx_is_initialized(void)
{
    return ((JSON_Parser != NULL) && (JSON_Parser->state == JX_INITIALIZED));
}
