#include "jx_api.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define JSONX_TEST_POOL_SIZE      2048U
#define JSONX_TEST_BUFFER_SIZE     256U

typedef struct
{
    char name[32];
    uint32_t position[2];
    uint32_t enabled;
} JsonX_TestModel;

static unsigned char jsonx_test_pool[JSONX_TEST_POOL_SIZE];

static int test_fail(const char *message)
{
    fprintf(stderr, "JsonX basic mapping test failed: %s\n", message);
    return 1;
}

int main(void)
{
    JsonX_TestModel model;
    char json_buffer[JSONX_TEST_BUFFER_SIZE];
    JX_STATUS status;

    memset(&model, 0, sizeof(model));
    strncpy(model.name, "Adam", sizeof(model.name) - 1U);
    model.position[0] = 12U;
    model.position[1] = 34U;
    model.enabled = 1U;

    JX_PROPERTY_U32_ARRAY_2(position_array, model.position);

    JX_ELEMENT root[] =
    {
        JX_PROPERTY_STRING_BUFFER("name", model.name),
        JX_PROPERTY_ARRAY("position", position_array),
        JX_PROPERTY_U32("enabled", model.enabled)
    };

    status = jx_init(jsonx_test_pool, sizeof(jsonx_test_pool));
    if (status != JX_SUCCESS)
    {
        return test_fail("jx_init");
    }

    status = jx_struct_to_json(root,
                               sizeof(root) / sizeof(root[0]),
                               json_buffer,
                               sizeof(json_buffer),
                               JX_MINIFIED);
    if (status != JX_SUCCESS)
    {
        jx_parser_deinit();
        return test_fail("jx_struct_to_json");
    }

    memset(&model, 0, sizeof(model));

    status = jx_json_to_struct(json_buffer,
                               root,
                               sizeof(root) / sizeof(root[0]),
                               JX_MODE_STRICT);
    if (status != JX_SUCCESS)
    {
        jx_parser_deinit();
        return test_fail("jx_json_to_struct");
    }

    if ((strcmp(model.name, "Adam") != 0) ||
        (model.position[0] != 12U) ||
        (model.position[1] != 34U) ||
        (model.enabled != 1U))
    {
        jx_parser_deinit();
        return test_fail("round-trip mismatch");
    }

    jx_parser_deinit();
    return 0;
}
