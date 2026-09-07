# CMake Toolchain file for cross-compiling to Windows x64 from macOS
# Usage: cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/mingw-w64-x64.cmake

# Target architecture
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

#mingw-w64 toolchain location
set(MINGW_PREFIX /opt/homebrew)

# Compiler binaries
set(CMAKE_C_COMPILER ${MINGW_PREFIX}/bin/x86_64-w64-mingw32-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_PREFIX}/bin/x86_64-w64-mingw32-g++)
set(CMAKE_RC_COMPILER ${MINGW_PREFIX}/bin/x86_64-w64-mingw32-windres)

# Search for programs only in the hosts
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target environment
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# Build in release mode by default
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CONFIGURATION_TYPES "Release" CACHE STRING "" FORCE)

# JUCE requires these
set(JUCE_COPY_PLUGIN_AFTER_BUILD OFF)

# Disable Direct2D and DirectWrite - use GDI+ for rendering
# This avoids issues with missing Direct2D headers in older mingw-w64
set(JUCE_USE_D2D1=0 CACHE BOOL "" FORCE)
set(JUCE_USE_DWRITE=0 CACHE BOOL "" FORCE)

# Allow 64-bit pointer casts
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fpermissive")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -std=gnu++20")
