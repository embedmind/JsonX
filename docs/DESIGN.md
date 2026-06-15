# JsonX Design

JsonX is a schema-driven JSON mapping library for embedded C.

## Core Contract

JsonX maps JSON values to caller-owned C storage described by `JX_ELEMENT`
declarations. A mapping is the schema and the storage binding at the same time:

- object properties are declared explicitly;
- primitive values point to caller-owned variables or buffers;
- strings declare destination capacity;
- arrays declare fixed capacity;
- nested objects and arrays compose the JSON tree.

The parser writes directly into mapped storage. The writer reads directly from
mapped storage. JsonX does not build a public DOM and does not own application
data.

## Intended Use Cases

- Firmware configuration files.
- Module manifests.
- Device identity/status documents with known fields.
- Protocol messages with fixed contracts.
- Embedded service request/response payloads where memory ownership must be explicit.

## Non-Goals

- Dynamic cJSON-like object trees as the primary API.
- Hidden heap allocation.
- Runtime schema discovery inside the mapping API.
- Parser-side rollback of arbitrary caller-owned storage.
- Full Unicode transcoding in the current byte-string backend.

## Atomicity

Parsing is non-transactional by design. If a JSON document writes several fields
successfully and then fails on a later field, earlier fields may already be
updated.

Callers that need atomic updates must parse into a candidate structure, validate
that candidate, and then activate it explicitly.

## Memory Model

JsonX has several integration modes:

- static bare-metal buffer;
- heap-backed bare-metal;
- ThreadX byte pool;
- FreeRTOS allocator;
- custom allocator hooks.

The preferred embedded mode is static bare-metal or an explicit RTOS pool. Public
APIs must not rely on hidden process heap allocation.

## Backend Boundary

Application code includes public headers from `inc/` only. Internal parser,
writer, debug, and allocator details live under `private/` and `src/`.

The current backend is native and self-contained. It must remain free of cJSON,
`strtok`, hidden heap use, `strtod()`, and `snprintf()` unless a future release
explicitly documents and justifies a change.

## Dynamic JSON Direction

Dynamic JSON is a valid future extension, but it belongs in a separate reader or
token API. The mapping API should stay schema-driven and deterministic.

See `DYNAMIC_JSON.md`.
