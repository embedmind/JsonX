/**************************************************************************/
/*                                                                        */
/*  @file example.c                                                       */
/*  @brief JsonX schema declaration example                               */
/*                                                                        */
/*  Shows how to declare readable JSON-shaped mappings using              */
/*  JX_ELEMENT helper macros and serialize/parse caller-owned storage.    */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#include "jx_api.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/*
 * Configuration model:
 *
 * JsonX includes inc/jx_config.h through public headers. Projects may provide
 * jx_user_config.h earlier on the include path to select integration mode and
 * optional parser features without editing library sources.
 *
 * Minimal ThreadX project override:
 *
 *   #define JX_DEBUG
 *   #define JX_USE_RTOS
 *   #define JX_USE_THREADX
 *   #define JX_ENABLE_JSON_COMMENTS 0
 *   #define JX_MAX_NESTING_LEVEL 3
 *   #define JX_PROPERTY_MAX_SIZE 50
 *
 * If no override is provided, JsonX defaults to static bare-metal mode.
 */

#ifdef JX_USE_CUSTOM_ALLOCATOR
#include <stdlib.h>
#endif

#define JX_MEM_POOL_SIZE     1024
#define JX_USER_BUFFER_SIZE  256
#define test_printf          printf

static char jx_byte_pool_buffer[JX_MEM_POOL_SIZE];
static TX_BYTE_POOL jx_byte_pool;

bool test(void)
{
    JX_STATUS state;
    test_printf("%s\r\n", jx_get_version_string());
#ifdef JX_USE_CUSTOM_ALLOCATOR
    // Initialize JsonX with custom memory hooks
    JX_HOOKS hooks = { .malloc_fn = malloc, .free_fn = free };
    state = jx_init(&hooks);
    if (state != JX_SUCCESS)
    {
        return false;
    }
#endif

#ifdef JX_USE_RTOS
    #ifdef JX_USE_THREADX
        // Create and initialize byte pool for ThreadX
        if (tx_byte_pool_create(&jx_byte_pool, "JsonX byte pool", jx_byte_pool_buffer, JX_MEM_POOL_SIZE) != TX_SUCCESS)
        {
            return false;
        }
        state = jx_init(&jx_byte_pool);
        if (state != JX_SUCCESS)
        {
            tx_byte_pool_delete(&jx_byte_pool);
            return false;
        }
    #elif defined(JX_USE_FREERTOS)
        // Initialize JsonX (FreeRTOS uses pvPortMalloc internally)
        state = jx_init();
        if (state != JX_SUCCESS)
        {
            return false;
        }
    #endif
#endif

#ifdef JX_USE_BAREMETAL
    #ifdef JX_USE_HEAP_BAREMETAL
        // Initialize JsonX (uses malloc/free internally)
        state = jx_init();
        if (state != JX_SUCCESS)
        {
            return false;
        }
    #else
        // Initialize JsonX with static buffer for allocation
        state = jx_init(jx_byte_pool_buffer, JX_MEM_POOL_SIZE);
        if (state != JX_SUCCESS)
        {
            return false;
        }
    #endif
#endif

    struct
    {
        char name[32];
        uint32_t position[2];
    } demo_user;

    JX_PROPERTY_U32_ARRAY_2(position_array, demo_user.position);

    JX_ELEMENT user_object[] =
    {
        JX_PROPERTY_STRING_BUFFER("name", demo_user.name),
        JX_PROPERTY_ARRAY("position", position_array)
    };

    size_t user_object_size = sizeof(user_object) / sizeof(user_object[0]);

    strncpy(demo_user.name, "Adam", sizeof(demo_user.name));
    demo_user.name[sizeof(demo_user.name) - 1U] = '\0';
    demo_user.position[0] = 12;
    demo_user.position[1] = 34;

    // Allocate buffer using JsonX allocation API
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
    char *user_buffer = jx_alloc_memory(JX_USER_BUFFER_SIZE);
    if (user_buffer == NULL)
	{
		jx_parser_deinit();
		return false;
	}
#else
    char user_buffer[JX_USER_BUFFER_SIZE];
#endif


    state = jx_struct_to_json(user_object, user_object_size, user_buffer, JX_USER_BUFFER_SIZE, JX_FORMATTED);
    if (state != JX_SUCCESS)
    {
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
        jx_free_memory(user_buffer);
#endif
        jx_parser_deinit();
        return false;
    }
    test_printf("Formatted JSON: %s\r\n", user_buffer);

    state = jx_struct_to_json(user_object, user_object_size, user_buffer, JX_USER_BUFFER_SIZE, JX_MINIFIED);
    if (state != JX_SUCCESS)
    {
#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
        jx_free_memory(user_buffer);
#endif
        jx_parser_deinit();
        return false;
    }
    test_printf("Minified JSON: %s\r\n", user_buffer);

#if !defined(JX_USE_BAREMETAL) || defined(JX_USE_HEAP_BAREMETAL)
    jx_free_memory(user_buffer);
#endif

    const char *input = "{\"name\":\"Eve\",\"position\":[56,78]}";
    state = jx_json_to_struct((char*)input, user_object, user_object_size, JX_MODE_STRICT);
    if (state != JX_SUCCESS)
    {
        jx_parser_deinit();
        return false;
    }
    test_printf("demo_user.name = %s\r\n", demo_user.name);
    test_printf("demo_user.position[0] = %d\r\n", (int)demo_user.position[0]);
    test_printf("demo_user.position[1] = %d\r\n", (int)demo_user.position[1]);

    jx_parser_deinit();
    return true;
}
