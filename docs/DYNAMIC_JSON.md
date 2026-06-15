# Dynamic JSON Roadmap

JsonX currently focuses on known JSON schemas mapped to caller-owned C storage.
Some applications may also need dynamic traversal of JSON documents whose shape
is not known at compile time.

## Can This Case Happen?

Yes. Examples:

- a web API endpoint accepts flexible payloads;
- a module manifest has an open-ended `metadata` object;
- a diagnostic command needs to inspect arbitrary JSON;
- a gateway forwards JSON from another system;
- a future scripting or plugin layer needs to scan optional fields.

These are valid use cases, but they should not force the core mapping API to
become a hidden dynamic DOM parser.

## Recommended Extension

Add a streaming/token reader API:

```c
typedef enum
{
    JX_TOKEN_ERROR = 0,
    JX_TOKEN_OBJECT_BEGIN,
    JX_TOKEN_OBJECT_END,
    JX_TOKEN_ARRAY_BEGIN,
    JX_TOKEN_ARRAY_END,
    JX_TOKEN_PROPERTY,
    JX_TOKEN_STRING,
    JX_TOKEN_U32,
    JX_TOKEN_I32,
    JX_TOKEN_U64,
    JX_TOKEN_I64,
    JX_TOKEN_BOOLEAN,
    JX_TOKEN_NULL
} JX_TOKEN_TYPE;
```

Possible public shape:

```c
JX_STATUS jx_reader_init(JX_READER *reader, const char *json, size_t length);
JX_STATUS jx_reader_next(JX_READER *reader, JX_TOKEN *token);
JX_STATUS jx_reader_skip_value(JX_READER *reader);
size_t jx_reader_error_offset(const JX_READER *reader);
```

Properties of this API:

- no public DOM allocation;
- no mutation of the input buffer;
- caller controls storage and token lifetime;
- unknown subtrees can be skipped cheaply;
- the same native parser primitives can be reused.

## Optional DOM Layer

An optional DOM can be added later on top of the reader if a real use case
requires it. If implemented, it should allocate nodes only from a caller-provided
pool or static arena and should be a separate feature from the mapping API.

## Current Policy

For known schemas, use `JX_ELEMENT` mappings.

For unknown schemas, do not abuse `JX_ELEMENT` as a runtime object tree. Add or
use the future reader API instead.
