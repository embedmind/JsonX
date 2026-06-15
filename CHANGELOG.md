# Changelog

All notable changes to JsonX are recorded here.

## 2.0.0-preview.1

Status: preview release candidate before stable standalone release.

### Added

- Native JsonX parser and serializer backend.
- Direct parsing and direct writing through `JX_ELEMENT` mappings.
- Typed integer mappings: `JX_U32`, `JX_I32`, `JX_U64`, and `JX_I64`.
- Explicit string capacity helpers such as `JX_PROPERTY_STRING_N` and `JX_PROPERTY_STRING_BUFFER`.
- Optional JSONC-style comment parsing through `JX_ENABLE_JSON_COMMENTS`.
- Parser error offset diagnostic through `jx_get_last_error_offset()`.
- Static bare-metal, ThreadX, heap-backed bare-metal, FreeRTOS, and custom allocator initialization modes.
- Public/private source layout for standalone packaging.
- CMake desktop build scaffold and basic smoke test.
- STM32CubeIDE ARM GCC Windows toolchain scaffold for cross-compile smoke builds.

### Changed

- `JX_ELEMENT` now keeps array capacity separate from current parsed/logical length.
- Legacy double-backed number support is opt-in through `JX_ENABLE_DOUBLE`.
- Static bare-metal initialization uses an explicit byte cursor and does not rely on compiler-specific `void *` arithmetic.

### Removed

- cJSON compatibility backend and cJSON source dependency.
- Backend value-tree compatibility API.
- Debug-only direct parser wrapper; regression tests use the production parse API.

### Known Limitations

- `JX_STATUS` is intentionally coarse and currently exposes only success/error.
- Dynamic JSON traversal is not a public API yet.
- Unicode escape output is limited to strict ASCII byte-string behavior.
