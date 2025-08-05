# 📦 JsonX — Lightweight Embedded JSON Serializer

JsonX — is a declarative C layer over popular [cJSON](https://github.com/DaveGamble/cJSON) library with embedded-first memory model and structured mapping via `JX_ELEMENT[]`.

---

## 🔧 Features

- ✅ ThreadX-compatible memory allocation via `TX_BYTE_POOL`
- ✅ FreeRTOS support via `pvPortMalloc()`/`vPortFree()`
- ✅ Baremetal support using either malloc or static buffer
- ✅ Custom allocator hooks support (`malloc/free` or user-defined)
- ✅ Clean `jx_` namespace in the style of Azure RTOS (`nx_`, `fx_`, etc.)
- ✅ Structured conversion between C data and JSON using `JX_ELEMENT`
- ✅ Support for complex nested objects and arrays
- ✅ Strict and non-strict parsing modes
- ✅ Optional logging with centralized `jx_log()` support
- ✅ Lightweight footprint (suitable for STM32 / Cortex-M targets)

---

## 📦 File Structure

| File                    | Description                                                       |
|-------------------------|-------------------------------------------------------------------|
| `jx_api.h`              | Public API headers (includes all public-facing declarations)      |
| `jx_config.h`           | Global configuration flags and compile-time feature toggles       |
| `jx_debug.h`            | Optional debug interface (logging abstraction)                    |
| `jx_internal.h`         | Internal interfaces and sanity checks (`JX_RETURN_IF_NULL`, etc.) |
| `jx_static_allocator.h` | Interface of static buffer allocator for baremetal mode           |
| `jx_types.h`            | Core type definitions and macros (enums, constants, structures)   |
| `jx_user.h`             | Helper macros for declaring arrays, objects, and element sets     |
| `jx_version.h`          | Library version definition and constants                          |
| `example.c`             | Demo usage with serialization of a `Person_t` structure           |
| `jx_parser.c`           | Core implementation: parsing, serialization, validation           |
| `jx_static_allocator.c` | Implementation of the static allocator                            |
| `jx_version.c`          | Version constant implementation                                   |
| `README.md`             | Project overview, usage examples, limitations, and build notes    |

---

## 🧠 Concepts

JsonX operates using a flat array of `JX_ELEMENT` structures. Each element represents a property or item in the JSON object, describing its type, memory location, and size.

Types supported:
- `JX_STRING`, `JX_NUMBER`, `JX_BOOLEAN`
- `JX_OBJECT` (nested), `JX_ARRAY` (list of elements)
- `JX_NULL`

---

## 💡 Design Philosophy

JsonX uses declarative macros to map static C structures directly into JSON-compatible formats. This allows memory-efficient, low-footprint mapping of nested objects, arrays, and strings without dynamic type inspection or reflection.

Key benefits:

- Avoids runtime heap usage in static mode
- Enables fully static definitions for embedded firmware
- Clear mapping between struct layout and resulting JSON

The `JX_ELEMENT[]` flat array acts as an intermediary representation for both parsing and serialization.

---

## ✨ Helper Macros

JsonX provides a set of declarative macros to simplify the definition of JSON ↔ C mappings.  
They are split into two categories:

### 🔹 Element Macros (for use inside arrays)

These do **not** include a `property` name and are intended for use inside `JX_PROPERTY_ARRAY(...)`, i.e., array elements:

| Macro             | Description |
|------------------|-------------|
| `JX_STRING_PTR(p)` | Maps a string pointer (`char *`). You manage the memory. |
| `JX_STRING_REF(v)` | Maps a string buffer (`char v[32]`). Expands to `&v`. |
| `JX_BOOLEAN_VAL(v)` | Maps a boolean value (`bool`, `uint8_t`, etc.). |
| `JX_NUMBER_VAL(v)`  | Maps a numeric value (int, float, double...). |
| `JX_ARRAY_VAL(arr)` | Maps a nested array (`JX_ELEMENT[]`). |
| `JX_OBJECT_VAL(obj)` | Maps a nested object (`JX_ELEMENT[]`). |
| `JX_OBJECT_EMPTY`    | Represents an empty object with no members. |

> 🔸 Use `_PTR` when you already have a pointer (`char *str`), and `_REF` when you have a fixed-size buffer (e.g., `char name[32]`).

---

### 🔹 Property Macros (for objects)

These include the property name (`"key"` in JSON) and are used when building JSON objects:

| Macro                            | Description |
|----------------------------------|-------------|
| `JX_PROPERTY_STRING(key, val)`   | `"key": string` mapping to `char[]` |
| `JX_PROPERTY_BOOLEAN(key, val)`  | `"key": true/false` mapping |
| `JX_PROPERTY_NUMBER(key, val)`   | `"key": number` mapping |
| `JX_PROPERTY_ARRAY(key, arr)`    | `"key": [...]` array of nested elements |
| `JX_PROPERTY_OBJECT(key, obj)`   | `"key": {}` nested object |
| `JX_PROPERTY_OBJECT_EMPTY(key)`  | `"key": {}` with no members |

---

### 🔹 Composite Array Macros

These macros help construct fixed-size arrays with multiple values or objects:

| Macro                                      | Description                                |
|-------------------------------------------|--------------------------------------------|
| `JX_PROPERTY_STRING_ARRAY_N(name, ref)`   | Declare an array of N strings (buffers)    |
| `JX_PROPERTY_NUMBER_ARRAY_N(name, ref)`   | Declare an array of N numbers              |
| `JX_PROPERTY_OBJECT_ARRAY_2(name, obj0, obj1)` | Declare a 2-element object array      |

> 🔸 All these macros expand into `JX_ELEMENT[]` declarations.  
> 🔸 `*_ARRAY_N(...)` macros help initialize common use cases like `[3][32]` string buffers.  
> 🔸 `*_OBJECT_ARRAY_N(...)` are ideal for grouping multiple sub-objects as array items.  

All composite helpers are defined in [`jx_user.h`](inc/jx_user.h), which is designed to hold such convenience macros.  
This file is fully under user control — modifying or extending it does **not** affect the core functionality of the library.

---

Using these helpers keeps your object mappings clean, readable, and maintainable.
---

## 📘 JX_ELEMENT Field Reference

Each JSON node is represented by the `JX_ELEMENT` structure:

```c
typedef struct json_element_s
{
    char                    property[JX_PROPERTY_MAX_SIZE];  // JSON key (property name)
    JX_ELEMENT_TYPE         type;                            // Data type: STRING, NUMBER, ARRAY, etc.
    uint8_t                 value_len;                       // Size of array/object (if applicable)
    const void             *value_p;                         // Pointer to value (primitive or buffer)
    JX_ELEMENT_STATUS       status;                          // Internal state: parsed, serialized, etc.
    struct json_element_s  *element;                         // Pointer to sub-elements (for object/array)
    uint16_t                element_size;                    // Number of sub-elements
} JX_ELEMENT;
```

> ⚠️ **Notes:**

> - property: Stores the JSON key (i.e., field name like "name", "id", etc).
> - 🔧 `JX_PROPERTY_MAX_SIZE` is defined in [`jx_config.h`](inc/jx_config.h).  
> You can adjust it to save RAM or support longer property names.  
> Example: set to `32` for compact mode, `64` for compatibility with long field names.
> - type: Enumerated value from JX_ELEMENT_TYPE (e.g., JX_STRING, JX_NUMBER, etc).
> - value_p: Pointer to primitive value (e.g., string/number/boolean).
> - element: For objects and arrays, points to the sub-elements.
> - value_len: Runtime length of the array or number of valid sub-elements (during parsing).
> - element_size: Static capacity (allocated size) of `element[]`. Required for parsing bounds checks.
> - status: Used internally by the parser/serializer to track element state.

> 🔸 All `JX_NUMBER` fields must be mapped to variables of type `double`.  
> This is a limitation inherited from the underlying [cJSON](https://github.com/DaveGamble/cJSON),  
> which stores all numeric values internally as `double`.

> 🔸 See `JX_ELEMENT_TYPE` and `JX_ELEMENT_STATUS` in [`jx_types.h`](inc/jx_types.h) for available constants.

This structure is the backbone of JsonX, enabling both serialization and deserialization of complex JSON structures.

---

## 🧪 Example: Full Nested Person Object

See [`example.c`](src/example.c) for a full demonstration of how to define nested JSON layouts using `JX_PROPERTY_OBJECT_ARRAY_2()` and other helpers.

---

## ⚙️ Initialization

Depending on system configuration, initialization differs. One of the following must be used depending on mode:

### ThreadX:

```c
TX_BYTE_POOL byte_pool;
CHAR byte_pool_buffer[1024];

tx_byte_pool_create(&byte_pool, "JsonX byte pool", byte_pool_buffer, sizeof(byte_pool_buffer));
jx_init(&byte_pool);
```

### FreeRTOS:

```c
jx_init();
```

### Baremetal with heap:

```c
jx_init();
```

### Baremetal with static buffer:

```c
CHAR static_buffer[1024];
jx_init(static_buffer, sizeof(static_buffer));
```

### Custom Allocator (platform-independent):

```c
JX_HOOKS hooks = {
    .alloc = malloc,
    .free = free
};
jx_init(&hooks);
```

---

## 🧩 Usage Example

```c
struct {
    char name[32];
    double coords[2];
} test_struct;

// JSON element array
JX_ELEMENT user[] = {
    { .type = JX_STRING, .property = "name", .value_p = test_struct.name },
    { .type = JX_ARRAY,  .property = "position", .element = (JX_ELEMENT[]) {
        JX_NUMBER_VAL(test_struct.coords[0]),
        JX_NUMBER_VAL(test_struct.coords[1])
    }, .value_len = 2 }
};
```
For a more advanced example of nested object arrays, see [example.c](src/example.c)

### Convert to JSON (with user-allocated buffer):

```c
char *json_buf = jx_alloc_memory(256);
jx_struct_to_json(user, 2, json_buf, 256, JX_FORMATTED);
```

### Parse from JSON:

```c
const char *input = "{\"name\":\"Eve\",\"position\":[12, 34]}";
jx_json_to_struct(input, user, 2, JX_MODE_STRICT);
```

### Output Result:

```c
test_printf("Name: %s, X: %d, Y: %d", test_struct.name, (int)test_struct.coords[0], (int)test_struct.coords[1]);
```

### Deinitialize:

```c
jx_free_memory(json_buf);
jx_parser_deinit();
```

---

## 🧼 Logging

If `JX_DEBUG` is set in `jx_config.h`, debug logging will be activated via the `jx_log(...)` macro:

```c
#define jx_log(...)  logger_printf(__VA_ARGS__)
```

Additionally, a debug-only utility function is available:

```c
void jx_dump_structure(JX_ELEMENT *desc, size_t count);
```

This function prints a human-readable tree of the current JSON structure — useful for verifying layout, element types, and parsing results.

> ⚠️ **Notes:**
> jx_dump_structure() is defined as static inline in jx_debug.h and compiled only when JX_DEBUG defined.
> No external linking or implementation is needed.
> This makes it lightweight and easy to enable/disable for debug builds.
---

### Known limitations

- In **baremetal static mode** (`JX_USE_BAREMETAL` without `JX_USE_HEAP_BAREMETAL`), `jx_alloc_memory()` must **not** be used by the user during JSON parsing or serialization.  
  Doing so may lead to undefined behavior due to cJSON_Delete() being internally called by JsonX, which uses the same static buffer allocator. This breaks allocator consistency when user code simultaneously allocates memory using jx_alloc_memory().
  Workaround: allocate required user buffers *before* calling JsonX APIs, or disable static mode using `JX_USE_HEAP_BAREMETAL`.

---

## 📌 Versioning

```c
#define JX_VERSION_MAJOR 1
#define JX_VERSION_MINOR 0
#define JX_VERSION_PATCH 0

const CHAR* jx_version(); // Returns static string: "JsonX v1.0.0 - (C) Mihail Zamurca, MIT Licensed"
```

---

## 👉 See also: [cJSON documentation]((https://github.com/DaveGamble/cJSON))

---

## 📋 License

JsonX uses cJSON under MIT license and wraps it for embedded systems.

© Mihail Zamurca — all rights reserved.