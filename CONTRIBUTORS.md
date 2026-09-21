# Contributing to sidecar

## Development Setup

sidecar is a C++17 application built with CMake. On Ubuntu or WSL, install the base build tools and the currently required PortAudio development package:

```bash
sudo apt update
sudo apt install build-essential cmake portaudio19-dev
```

Configure and build from the `sidecar` directory:

```bash
cmake -S . -B build
cmake --build build
```

## Adding and Installing a Package

1. Identify the system package that provides the library's headers and development files. On Ubuntu, development packages commonly end in `-dev`.
2. Install it with `apt`:

   ```bash
   sudo apt update
   sudo apt install <package-name>-dev
   ```

3. Add the package's header to the relevant source file, usually with angle brackets:

   ```cpp
   #include <library-header.h>
   ```

4. Update `CMakeLists.txt` so CMake can find the header and library. For a library with standard include and library locations, use the existing pattern:

   ```cmake
   find_path(LIBRARY_INCLUDE_DIR library-header.h REQUIRED)
   find_library(LIBRARY_LIBRARY library REQUIRED)

   target_include_directories(sidecar PRIVATE ${LIBRARY_INCLUDE_DIR})
   target_link_libraries(sidecar PRIVATE ${LIBRARY_LIBRARY})
   ```

   Replace the placeholder names with the package-specific header, library, and CMake variable names.

5. Reconfigure and build to verify the package is available:

   ```bash
   cmake -S . -B build
   cmake --build build
   ```

6. If the package is no longer needed, remove its source include, CMake entries, and package installation instructions.

For PortAudio, the package is `portaudio19-dev`, the header is `portaudio.h`, and the library name used by CMake is `portaudio`.

## Before Submitting Changes

Build and run the application:

```bash
cmake --build build
./build/sidecar
```

Keep generated files in `build/`; do not add the build directory's generated contents to source control.
