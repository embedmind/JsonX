# JsonX Standalone Release Checklist

Use this checklist before publishing JsonX as a standalone GitHub repository.

## Public Positioning

- State that JsonX is schema-driven and maps known JSON structures to C storage.
- State that JsonX is not currently a dynamic DOM parser.
- Document the planned dynamic reader/token API separately.
- Keep firmware safety and deterministic-memory claims specific and test-backed.

## Source Layout

- Keep public headers under `inc/`.
- Keep internal headers under `private/`.
- Keep implementation files under `src/`.
- Keep examples under `examples/`.
- Keep desktop tests under `tests/`.
- Keep public release docs project-neutral unless the repository is intentionally branded for a specific firmware project.

## License

- Confirm the intended public license before release.
- Current license file is MIT.
- Ensure copyright owner and years are correct.

## Build And Tests

- Build with CMake on a desktop compiler.
- Run `ctest` for standalone smoke tests.
- Build at least one static bare-metal profile.
- Build the ARM GCC cross profile with `cmake/toolchain-arm-gcc-win.cmake` and `ARM_GCC_ROOT` supplied by CI/environment.
- Build one RTOS/profile integration sample if ThreadX or FreeRTOS headers are available.
- Run downstream firmware/application regression tests after any parser or writer behavior change.

## API Stability

- Treat `jx_api.h`, `jx_types.h`, `jx_user.h`, `jx_config.h`, and `jx_version.h` as public.
- Do not expose `private/` headers.
- Keep dynamic JSON traversal out of the mapping API unless a new public contract is written.
- Document any future breaking change in `CHANGELOG.md`.

## Release Artifacts

- README has quick start and limitations.
- `CHANGELOG.md` has the release entry.
- `LICENSE` is present and correct.
- CMake build works from the repository root.
- Examples compile or are clearly marked as illustrative.
- Version constants match the release tag.
