# JsonX

JsonX is a small schema-driven JSON mapping library for embedded C firmware.
It maps JSON documents into caller-owned C storage through explicit `JX_ELEMENT`
trees and writes JSON back from the same declarations.

JsonX is designed for firmware configuration, manifests, protocol messages, and
other JSON shapes that are known at compile time. It is intentionally not a
general-purpose dynamic JSON DOM parser.

The current implementation uses the native JsonX parser/serializer backend
behind an internal backend adapter. Application code should include only
`jx_api.h` and should not include backend headers directly.

## What JsonX Is

- Declarative JSON-to-C mapping for known schemas.
- Direct parser and writer over caller-owned storage.
- Embedded-friendly and deterministic by default.
- Heap-free in static bare-metal mode.
- RTOS-friendly through optional ThreadX integration.
- Strict JSON by default, with optional JSONC-style comments for selected builds.

## What JsonX Is Not

- Not a cJSON-compatible replacement API.
- Not currently a dynamic DOM parser.
- Not a scripting/object database layer.
- Not transactional by itself; callers that need atomic updates must parse into candidate storage first.

Dynamic JSON traversal is a valid future use case, but it should be added as a
separate token/reader API rather than by turning the mapping API into a hidden
DOM. See [`docs/DYNAMIC_JSON.md`](docs/DYNAMIC_JSON.md).

## Public Headers

| Header | Role |
|---|---|
| `jx_api.h` | Public initialization, allocation, parse, and serialize API. |
| `jx_types.h` | Public enums, `JX_ELEMENT`, and primitive mapping macros. |
| `jx_user.h` | User-facing helper macros for fixed-size arrays and object arrays. |
| `jx_config.h` | Compile-time configuration boundary and library defaults. |
| `jx_version.h` | Version constants and version query API. |

Internal headers live in `private/` and are implementation details. Application modules should not include `jx_internal.h`, `jx_debug.h`, `jx_backend.h`, or `jx_static_allocator.h`.

The native backend is implemented by `src/jx_native_backend.c`. The older cJSON compatibility backend has been removed.

## Preview Status

Current preview: `2.0.0-preview.1`.

This directory contains the JsonX v2 preview source prepared for standalone
publication. The preview is self-contained except for optional RTOS
integration.

Before release, keep these boundaries intact:

- application code includes public headers from `inc/` only;
- internal headers stay in `private/`;
- project policy lives in a project-provided `jx_user_config.h`;
- the native backend remains free of cJSON, `strtok`, hidden heap use, `strtod()`, and `snprintf()`;
- config-backed firmware regression tests remain the safety net for parser and writer changes.

Standalone release preparation files:

| File | Role |
|---|---|
| `CHANGELOG.md` | Release history and migration notes. |
| `LICENSE` | MIT license. |
| `CMakeLists.txt` | Desktop/static-library build scaffold. |
| `cmake/toolchain-arm-gcc-win.cmake` | ARM GCC cross-compile scaffold for Windows. |
| `docs/DESIGN.md` | Public design contract and non-goals. |
| `docs/DYNAMIC_JSON.md` | Planned dynamic JSON reader direction. |
| `docs/RELEASE_CHECKLIST.md` | Checklist before publishing a standalone repository. |
| `tests/` | Desktop smoke tests for the standalone build. |

## Source Layout

| Path | Role |
|---|---|
| `inc/` | Public headers intended for application code. |
| `private/` | Internal headers shared only by JsonX implementation files. |
| `src/` | JsonX implementation units. |
| `examples/` | Non-production usage examples excluded from the firmware build. |

## Mapping Model

JsonX uses `JX_ELEMENT[]` declarations to describe the JSON document shape. Every element describes:

- JSON property name, when the element is part of an object;
- JSON value type;
- pointer to caller-owned primitive storage or nested elements;
- fixed string buffer capacity, when the element is a string;
- object element count or current logical array length;
- fixed array capacity, when the element is an array;
- parser update status.

Integer configuration values should use typed mappings such as `JX_U32`, `JX_I32`, `JX_U64`, and `JX_I64`. Legacy `JX_NUMBER`/`double` mappings are available only when `JX_ENABLE_DOUBLE` is enabled through the compile-time configuration.

## Compile-Time Configuration

JsonX ships with safe library defaults in `inc/jx_config.h`. Projects that need a different integration mode or parser feature set should provide a `jx_user_config.h` on the compiler include path before `Libs/JsonX/inc`.

The project override owns project/profile policy, for example RTOS selection, debug logging, and optional parser compatibility flags. The library defaults remain usable when no override is provided.

Example project override:

```c
#ifndef JX_USER_CONFIG_H_
#define JX_USER_CONFIG_H_

#define JX_DEBUG

#define JX_USE_RTOS
#define JX_USE_THREADX

/* Keep legacy double-backed numbers disabled unless fractional JSON is required. */
#define JX_ENABLE_DOUBLE 0

/* Strict JSON by default. Set to 1 only for profiles that accept JSONC input. */
#define JX_ENABLE_JSON_COMMENTS 0

#define JX_MAX_NESTING_LEVEL 3
#define JX_PROPERTY_MAX_SIZE 50

#endif /* JX_USER_CONFIG_H_ */
```

Current options:

| Option | Default | Behavior |
|---|---:|---|
| `JX_USE_BAREMETAL` | enabled if no override selects an integration mode | Uses a caller-provided static buffer unless `JX_USE_HEAP_BAREMETAL` is also enabled. |
| `JX_USE_RTOS` | disabled | Uses an RTOS-backed allocator. Select `JX_USE_THREADX` or `JX_USE_FREERTOS`; ThreadX is assumed if RTOS mode is selected without a concrete RTOS. |
| `JX_USE_CUSTOM_ALLOCATOR` | disabled | Uses caller-provided allocation hooks. |
| `JX_DEBUG` | disabled | Enables internal debug helpers/log output. |
| `JX_ENABLE_DOUBLE` | `0` | Enables legacy `JX_NUMBER` / `double` mappings when set to `1`. |
| `JX_ENABLE_JSON_COMMENTS` | `0` | When set to `1`, the native parser accepts `//` line comments and C-style block comments outside strings. The writer always emits strict JSON without comments. |
| `JX_MAX_NESTING_LEVEL` | `3` | Maximum nested object/array depth accepted by the native parser. |
| `JX_PROPERTY_MAX_SIZE` | `50` | Maximum JSON property-name buffer size and legacy fallback string capacity. Prefer explicit string-capacity macros for mapped string buffers. |

## Basic Example

```c
#include "jx_api.h"

#include <stdint.h>

typedef struct
{
    char name[32];
    uint32_t position[2];
} DemoUser_t;

static DemoUser_t demo_user;

JX_PROPERTY_U32_ARRAY_2(position_array, demo_user.position);

static JX_ELEMENT user_object[] =
{
    JX_PROPERTY_STRING_BUFFER("name", demo_user.name),
    JX_PROPERTY_ARRAY("position", position_array)
};

static const size_t user_object_size = sizeof(user_object) / sizeof(user_object[0]);
```

Serialize:

```c
char json_buffer[256];

if (jx_struct_to_json(user_object,
                      user_object_size,
                      json_buffer,
                      sizeof(json_buffer),
                      JX_FORMATTED) != JX_SUCCESS)
{
    /* Handle serialization failure. */
}
```

Parse:

```c
char json_buffer[] = "{\"name\":\"Eve\",\"position\":[56,78]}";

if (jx_json_to_struct(json_buffer,
                      user_object,
                      user_object_size,
                      JX_MODE_STRICT) != JX_SUCCESS)
{
    size_t error_offset = jx_get_last_error_offset(json_buffer);
    /* Handle parse failure. error_offset is (size_t)-1 when unknown. */
}
```

## Standalone Build

Desktop/native build:

```sh
cmake -S . -B build -DJSONX_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

STM32CubeIDE ARM GCC cross-compile smoke build on Windows:

```sh
set ARM_GCC_ROOT=D:/path/to/arm-gnu-toolchain

cmake -S . -B build-arm ^
  -G "Ninja" ^
  -DCMAKE_TOOLCHAIN_FILE="cmake/toolchain-arm-gcc-win.cmake" ^
  -DJSONX_BUILD_TESTS=ON

cmake --build build-arm
```

`ARM_GCC_ROOT` can also be provided as an environment variable, which is the
preferred path for CI workflows such as GitHub Actions.

The ARM cross build can compile and link the smoke-test executable, but `ctest`
does not register it because the target binary cannot run on the Windows host.

## Parse Atomicity

JsonX parses directly into caller-owned storage referenced by `JX_ELEMENT`. This keeps the library small and deterministic, but parsing is not transactional: fields that were successfully parsed before a later syntax, type, capacity, or validation error may already have been written to the mapped storage.

Callers that need atomic updates must parse into candidate/shadow storage first, validate the complete candidate, and only then copy or activate it. Do not map `jx_json_to_struct()` directly to live configuration, driver state, safety state, or any storage that cannot tolerate partial updates.

## Initialization

Exactly one integration mode must be selected either by `jx_user_config.h` or by the defaults in `jx_config.h`.

ThreadX:

```c
TX_BYTE_POOL byte_pool;
UCHAR byte_pool_buffer[1024];

tx_byte_pool_create(&byte_pool,
                    "JsonX pool",
                    byte_pool_buffer,
                    sizeof(byte_pool_buffer));

jx_init(&byte_pool);
```

FreeRTOS or heap-backed bare-metal:

```c
jx_init();
```

Static bare-metal:

```c
UCHAR static_buffer[1024];

jx_init(static_buffer, sizeof(static_buffer));
```

The static bare-metal backend treats this buffer as byte-addressable storage and does not rely on compiler-specific `void *` pointer arithmetic.

Custom allocator:

```c
JX_HOOKS hooks =
{
    .malloc_fn = my_alloc,
    .free_fn = my_free
};

jx_init(&hooks);
```

Call `jx_parser_deinit()` before reinitializing JsonX or during shutdown.

## Helper Macros

Primitive value macros:

| Macro | Use |
|---|---|
| `JX_STRING_PTR(value)` | Legacy string array item from a pointer-like value using `JX_PROPERTY_MAX_SIZE` as fallback capacity. |
| `JX_STRING_REF(value)` | Legacy string array item from a fixed buffer reference using `JX_PROPERTY_MAX_SIZE` as fallback capacity. |
| `JX_STRING_PTR_N(value, capacity)` | String array item from a pointer-like value with explicit capacity. |
| `JX_STRING_REF_N(value, capacity)` | String array item from a fixed buffer reference with explicit capacity. |
| `JX_STRING_BUFFER(buffer)` | String array item from a fixed buffer; capacity is inferred with `sizeof(buffer)`. |
| `JX_BOOLEAN_VAL(value)` | Boolean array item. |
| `JX_U32_VAL(value)` | Unsigned 32-bit integer array item. |
| `JX_I32_VAL(value)` | Signed 32-bit integer array item. |
| `JX_U64_VAL(value)` | Unsigned 64-bit integer array item. |
| `JX_I64_VAL(value)` | Signed 64-bit integer array item. |
| `JX_NUMBER_VAL(value)` | Legacy double-backed numeric array item. Declared only when `JX_ENABLE_DOUBLE == 1`. |
| `JX_ARRAY_VAL(elements)` | Nested array item. |
| `JX_OBJECT_VAL(elements)` | Nested object item. |
| `JX_OBJECT_VAL_N(elements, count)` | Nested object item with explicit element count. |

Object property macros:

| Macro | Use |
|---|---|
| `JX_PROPERTY_STRING(name, value)` | Legacy string property using `JX_PROPERTY_MAX_SIZE` as fallback capacity. |
| `JX_PROPERTY_STRING_N(name, value, capacity)` | String property with explicit capacity. |
| `JX_PROPERTY_STRING_BUFFER(name, buffer)` | String property from a fixed buffer; capacity is inferred with `sizeof(buffer)`. |
| `JX_PROPERTY_BOOLEAN(name, value)` | Boolean property. |
| `JX_PROPERTY_U32(name, value)` | Unsigned 32-bit integer property. |
| `JX_PROPERTY_I32(name, value)` | Signed 32-bit integer property. |
| `JX_PROPERTY_U64(name, value)` | Unsigned 64-bit integer property. |
| `JX_PROPERTY_I64(name, value)` | Signed 64-bit integer property. |
| `JX_PROPERTY_NULL(name)` | Null marker property. It validates/publishes JSON `null` and does not write to mapped storage. |
| `JX_PROPERTY_NUMBER(name, value)` | Legacy double-backed numeric property. Declared only when `JX_ENABLE_DOUBLE == 1`. |
| `JX_PROPERTY_ARRAY(name, elements)` | Array property. |
| `JX_PROPERTY_ARRAY_N(name, elements, count)` | Array property with explicit logical count. |
| `JX_PROPERTY_OBJECT(name, elements)` | Object property. |
| `JX_PROPERTY_OBJECT_EMPTY(name)` | Empty object property. |

Fixed-size helpers in `jx_user.h`:

| Macro | Use |
|---|---|
| `JX_PROPERTY_NUMBER_ARRAY_2(name, array)` | Declare two legacy double-backed numeric array items. Declared only when `JX_ENABLE_DOUBLE == 1`. |
| `JX_PROPERTY_NUMBER_ARRAY_3(name, array)` | Declare three legacy double-backed numeric array items. Declared only when `JX_ENABLE_DOUBLE == 1`. |
| `JX_PROPERTY_NUMBER_ARRAY_4(name, array)` | Declare four legacy double-backed numeric array items. Declared only when `JX_ENABLE_DOUBLE == 1`. |
| `JX_PROPERTY_U32_ARRAY_2(name, array)` | Declare two unsigned 32-bit integer array items. |
| `JX_PROPERTY_U32_ARRAY_3(name, array)` | Declare three unsigned 32-bit integer array items. |
| `JX_PROPERTY_U32_ARRAY_4(name, array)` | Declare four unsigned 32-bit integer array items. |
| `JX_PROPERTY_U64_ARRAY_2(name, array)` | Declare two unsigned 64-bit integer array items. |
| `JX_PROPERTY_U64_ARRAY_3(name, array)` | Declare three unsigned 64-bit integer array items. |
| `JX_PROPERTY_U64_ARRAY_4(name, array)` | Declare four unsigned 64-bit integer array items. |
| `JX_PROPERTY_STRING_ARRAY_2(name, array)` | Declare two string array items. |
| `JX_PROPERTY_STRING_ARRAY_3(name, array)` | Declare three string array items. |
| `JX_PROPERTY_OBJECT_ARRAY_2(name, object0, object1)` | Declare two object array items. |
| `JX_PROPERTY_OBJECT_ARRAY_6(name, object0, object1, object2, object3, object4, object5)` | Declare six object array items. |

## Current Limitations

- `JX_STATUS` currently exposes only `JX_SUCCESS` and `JX_ERROR`.
- `jx_get_last_error_offset()` exposes the last parser error position for diagnostics, but detailed typed error codes are not implemented yet.
- Dynamic JSON traversal is not part of the public API yet. Use declarative mappings for known schemas. Future dynamic use should go through a token/reader API, not through a hidden heap-backed DOM.
- Firmware config mappings use typed integer values. Legacy double-backed number support is opt-in through `JX_ENABLE_DOUBLE`.
- Legacy `JX_NUMBER` helper macros are declared only when `JX_ENABLE_DOUBLE == 1`; disabled builds should fail at compile time if new double-backed mappings are introduced.
- Project-specific configuration should live in `jx_user_config.h`; avoid editing library defaults for product/profile policy.
- String mappings should use explicit capacity through `JX_PROPERTY_STRING_N`, `JX_PROPERTY_STRING_BUFFER`, `JX_STRING_PTR_N`, `JX_STRING_REF_N`, or `JX_STRING_BUFFER`. Legacy string macros keep `JX_PROPERTY_MAX_SIZE` as fallback capacity.
- Strict mode rejects missing fields, wrong present types, oversized arrays, and oversized strings.
- Relaxed mode skips missing fields and clears the element status for wrong present types, but still rejects memory-safety boundary violations.
- JSON comments are rejected by default. `JX_ENABLE_JSON_COMMENTS` can enable JSONC-style comments for selected builds, but serializer output remains strict JSON.
- The native backend parses and writes directly through declared `JX_ELEMENT` mappings.
- Array mappings keep `value_capacity` as fixed capacity and `value_len` as current logical/parsed item count.
- Parsing is direct and non-transactional. Atomic updates require caller-owned candidate storage and explicit activation after validation.
- String parsing accepts simple JSON escapes and ASCII `\u0001`..`\u007F` escapes. `\u0000`, control-code escapes, and non-ASCII Unicode escapes are rejected until UTF-8 output support is implemented.
- The native integer parser/formatter is intentionally small and heap-free. Integer mappings reject fractional, exponent, negative-for-unsigned, and overflowed values.
- `JX_NULL` is marker-only. Parsing `null` marks the element updated but does not modify caller storage.
- `jx_debug.h` remains an internal/debug header.

## Version

```c
const char *jx_get_version_string(void);
```

The library version constants are declared in `jx_version.h`.

Current preview marker:

```c
#define JX_VERSION_MAJOR       2
#define JX_VERSION_MINOR       0
#define JX_VERSION_PATCH       0
#define JX_VERSION_PRERELEASE  "preview.1"
```

`jx_get_version_string()` returns a human-readable preview release string suitable for debug logs and version diagnostics.
