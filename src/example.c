/**************************************************************************/
/*                                                                        */
/*  @file example.c                                                       */
/*  @brief Demonstration of JsonX with Nested Object Layouts              */
/*                                                                        */
/*  Shows how to declare complex nested object layouts using              */
/*  JX_ELEMENT macros and serialize/parse JSON structures.                */
/*                                                                        */
/*  @author Mihail Zamurca                                                */
/*                                                                        */
/**************************************************************************/

#include "jx_api.h"

#define JX_MEM_POOL_SIZE     1024
#define JX_USER_BUFFER_SIZE  256
#define test_printf          printf

static char jx_byte_pool_buffer[JX_MEM_POOL_SIZE];
static TX_BYTE_POOL jx_byte_pool;

bool test(void)
{
    JX_STATUS state;
    jx_log("%s\r\n", jx_get_version_string());
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

    // Define test structure with name and coordinate array
    struct
    {
        char name[32];
        double coords[2];
    } test_struct;

    // Bind JSON fields to structure fields
    JX_ELEMENT user_object[] = {
        { .type = JX_STRING, .property = "name", .value_p = test_struct.name },
        { .type = JX_ARRAY,  .property = "position", .element = (JX_ELEMENT[]) {
            JX_NUMBER_VAL(test_struct.coords[0]),
            JX_NUMBER_VAL(test_struct.coords[1])
        }, .value_len = 2 }
    };
    size_t user_object_size = sizeof(user_object) / sizeof(user_object[0]);

    // Initialize test data
    strncpy(test_struct.name, "Adam", sizeof(test_struct.name));
    test_struct.coords[0] = 12;
    test_struct.coords[1] = 34;

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


    // Convert structure to formatted JSON
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

    // Convert structure to minified JSON
    state = jx_struct_to_json(user_object, user_object_size, user_buffer, JX_USER_BUFFER_SIZE, JX_MINIFIED);
    if (state != JX_SUCCESS)
    {
        jx_free_memory(user_buffer);
        jx_parser_deinit();
        return false;
    }
    test_printf("Minified JSON: %s\r\n", user_buffer);

    // Free buffer after conversion
    jx_free_memory(user_buffer);

    // Parse JSON into structure
    const char *input = "{\"name\":\"Eve\",\"position\":[56,78]}";
    state = jx_json_to_struct((char*)input, user_object, user_object_size, JX_MODE_STRICT);
    if (state != JX_SUCCESS)
    {
        jx_parser_deinit();
        return false;
    }
    jx_dump_structure(user_object, user_object_size);
    // Output parsed structure contents
    test_printf("test_struct.name = %s\r\n", test_struct.name);
    test_printf("test_struct.coords[0] = %d\r\n", (int)test_struct.coords[0]);
    test_printf("test_struct.coords[1] = %d\r\n", (int)test_struct.coords[1]);

    // Deinitialize JsonX (cleans up global state)
    jx_parser_deinit();
    return true;
}
