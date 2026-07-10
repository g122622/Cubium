# Cubium - Copilot Coding Agent Instructions

## Project Overview

Cubium is a C++20 Minecraft clone with a client-server architecture,
targeting Java Edition 1.16.5 compatibility. The codebase is ~1M+ lines of C++.

## Tech Stack

- **Language:** C++20 (strict), Clang compiler
- **Build System:** CMake 3.26+ with vcpkg for dependency management
- **Formatting:** clang-format (config in `.clang-format` at repo root)
- **Testing:** Google Test (ctest runner)
- **Sanitizers:** ASan, UBSan, TSan enabled in CI
- **Architecture:** `src/client/`, `src/common/`, `src/server/`, `tests/`

## Critical Build Commands

### Linux (CI and development)

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DMC_BUILD_CLIENT=OFF \
  -DMC_BUILD_SERVER=ON \
  -DMC_BUILD_TESTS=ON \
  -DMC_ENABLE_SANITIZERS=OFF \
  -DMC_ENABLE_CCACHE=ON \
  -DVCPKG_TARGET_TRIPLET=x64-linux

cmake --build build -j$(nproc)
cd build && ctest --output-on-failure --timeout 300 -j$(nproc)
```

### ASan + UBSan build

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMC_BUILD_CLIENT=OFF \
  -DMC_BUILD_SERVER=ON \
  -DMC_BUILD_TESTS=ON \
  -DMC_ENABLE_SANITIZERS=ON \
  -DMC_ENABLE_CCACHE=ON \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all" \
  -DCMAKE_C_FLAGS="-fsanitize=address,undefined -fno-sanitize-recover=all" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address,undefined" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=address,undefined"
```

### TSan build

```bash
cmake -B build -G Ninja \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_BUILD_TYPE=Debug \
  -DMC_BUILD_CLIENT=OFF \
  -DMC_BUILD_SERVER=ON \
  -DMC_BUILD_TESTS=ON \
  -DMC_ENABLE_SANITIZERS=ON \
  -DMC_ENABLE_CCACHE=ON \
  -DVCPKG_TARGET_TRIPLET=x64-linux \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -fno-sanitize-recover=thread" \
  -DCMAKE_C_FLAGS="-fsanitize=thread -fno-sanitize-recover=thread" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread" \
  -DCMAKE_SHARED_LINKER_FLAGS="-fsanitize=thread"
```

### Windows (CI only)

```cmd
call "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\Common7\Tools\VsDevCmd.bat" -arch=amd64 -host_arch=amd64 -no_logo
cmake -B build -G Ninja Multi-Config -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DMC_BUILD_CLIENT=ON -DMC_BUILD_SERVER=ON -DMC_BUILD_TESTS=OFF -DMC_ENABLE_SANITIZERS=OFF -DVCPKG_TARGET_TRIPLET=x64-windows
cmake --build build --config RelWithDebInfo
```

### Format check

```bash
clang-format --style=file -i <path-to-file>
```

## Code Conventions (MUST follow)

- **Naming:** PascalCase for types, `m_` prefix + camelCase for members, camelCase for locals, UPPER_SNAKE_CASE for constants
- **Namespaces:** All code in `mc::` namespace (mc::client, mc::server, mc::common sub-namespaces)
- **No `using namespace std;`** ever
- **No C-style casts** - use static_cast, reinterpret_cast, etc.
- **No raw new/delete** - use std::make_unique, std::make_shared
- **No C-style arrays** - use std::array or std::vector
- **No exceptions** - use Result<T> for error handling
- **Use f32 not f64** unless precision is explicitly needed
- **Private methods** must have `_` prefix
- **Comments** must be in Simplified Chinese
- **Doxygen** comments only in header files
- **Include paths:** Use `#include "path/from/src/..."` not `../` relative paths
- **Use `[[nodiscard]]`** on functions returning Result or owning pointers
- **Use `MC_ASSERT_RELEASE`** for runtime assertions, never defensive null checks that hide bugs
- **Use `mc::math::Random`** for all random number generation, never std::mt19937
- **Use constants from** `mc::world::`, `mc::game::`, `mc::network::` namespaces -- never hardcode magic numbers for chunk sizes, build heights, etc.
- **Use `MC_TRACE_SCOPED_EVENT` / `MC_TRACE_COUNTER`** for performance tracing. The project runs Perfetto + Tracy dual-track (both on by default; toggle via `-DMC_ENABLE_TRACING` / `-DMC_ENABLE_TRACY`). The category must be a leaf of the `mc::trace::TraceEvents` enum tree (defined in `src/common/profiler/TraceCategories.hpp`), e.g. `MC_TRACE_SCOPED_EVENT(TraceEvents.Server.Tick, "ServerWorld::tick")`. Categories are matched at compile time (Perfetto side), so an unregistered category fails to compile. Dual-track macros expand to multiple statements, so do not wrap them in `EXPECT_NO_THROW(...)`-style single-statement macros.
- **No default parameter values** without explicit approval
- **Anonymous namespaces** for file-local symbols instead of `static` or global namespace

## IMPORTANT: Do NOT do these

- Do NOT add defensive null checks that hide real bugs. The project philosophy is: let bugs crash visibly so they can be fixed.
- Do NOT leave "align MC" or "参考 MC" style comments. Remove any you find.
- Do NOT use default parameter values without explicit approval.
- Do NOT create README.md files in the tests/ directory.
- Do NOT use TODO/FIXME without describing what needs to be done.
- Do NOT modify the `.clang-format` file.
- Do NOT modify `vcpkg.json` or `CMakeLists.txt` unless the fix explicitly requires a dependency or build change.
- Do NOT use `../` in include paths. Use the full path from `src/` instead.
- Do NOT use `using namespace std;`.
- Do NOT write debug-level log messages (only info level and above are visible).

## Fixing CI Failures

When assigned an auto-remediation issue, follow this process:

### Formatting failures

1. Read the format-check job log to identify affected files.
2. Run `clang-format --style=file -i <file>` on each affected file.
3. Commit with message: `fix: apply clang-format to affected files`
4. Do NOT change any other code.

### Build errors

1. Read the build log carefully. Identify the root cause (missing include, type mismatch, linker error).
2. Fix ONLY the compilation error. Do not refactor or change unrelated code.
3. If the fix requires adding a missing `#include`, add it at the correct position per `.clang-format` sort order.
4. Run clang-format on any modified files.
5. Commit with message: `fix: resolve build error - <brief description>`

### Test failures

1. Read the ctest output. Identify the failing test and assertion.
2. Analyze whether the test expectation is correct or the implementation has a bug.
3. Fix the implementation (preferred) or update the test if the expectation was wrong.
4. Do NOT disable or skip failing tests.
5. Commit with message: `fix: resolve test failure in <test name>`

### Sanitizer violations

1. Read the sanitizer output carefully. ASan/UBSan/TSan reports include stack traces.
2. Identify the root cause (use-after-free, buffer overflow, data race, etc.).
3. Fix the memory or threading issue. Common patterns:
   - Use smart pointers instead of raw pointers
   - Add proper synchronization for shared data
   - Initialize variables before use
   - Fix buffer bounds
4. Commit with message: `fix: resolve <sanitizer-type> violation in <component>`

### Infrastructure failures

1. Do NOT attempt to fix these automatically. Comment on the issue explaining
   the infrastructure problem and tag a maintainer.

## PR Requirements

- All PRs must target the branch specified in the issue (usually `main`).
- PR title must start with `fix:`, `feat:`, or `refactor:`.
- PR description must reference the auto-remediation issue number.
- Do NOT auto-merge. All PRs require human review.
- Run clang-format on ALL modified files before committing.
