cmake_minimum_required(VERSION 3.20)
project(PardiseSolver CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =============================================================================
# Path to your Panua Pardiso installation.
# Override from the command line with:  -DPARDISO_DIR=/path/to/pardiso
# =============================================================================
if(NOT DEFINED PARDISO_DIR)
    if(DEFINED ENV{PARDISO_DIR})
        set(PARDISO_DIR "$ENV{PARDISO_DIR}")
    else()
        message(FATAL_ERROR
            "PARDISO_DIR is not set. Pass it via -DPARDISO_DIR=... "
            "or set the PARDISO_DIR environment variable.\n"
            "Example: cmake -B build -DPARDISO_DIR=C:/path/to/panua-pardiso")
    endif()
endif()
message(STATUS "Using Pardiso from: ${PARDISO_DIR}")


# =============================================================================
# Find the Pardiso library (platform-specific filenames)
# =============================================================================
if(WIN32)
    set(PARDISO_LIB     "${PARDISO_DIR}/lib/libpardiso.lib")
    set(PARDISO_DLL     "${PARDISO_DIR}/lib/libpardiso.dll")
    set(PARDISO_OMP_DLL "${PARDISO_DIR}/lib/libiomp5md.dll")
else()
    # On Linux: libpardiso.so (sometimes named libpardiso800-LINUX-X86-64.so)
    find_library(PARDISO_LIB
        NAMES pardiso libpardiso
        PATHS "${PARDISO_DIR}/lib" "${PARDISO_DIR}"
        REQUIRED)
endif()

# =============================================================================
# Sources
# =============================================================================
add_executable(pardiso_solver
    main.cpp
    matrix_loader.cpp
    pardiso_solver.cpp
)

# Include directory (if Panua ships headers)
if(EXISTS "${PARDISO_DIR}/include")
    target_include_directories(pardiso_solver PRIVATE "${PARDISO_DIR}/include")
endif()

# =============================================================================
# Platform-specific linking
# =============================================================================
if(WIN32)
    # MSVC: just link the .lib, DLLs are copied next to the .exe (see below)
    target_link_libraries(pardiso_solver PRIVATE "${PARDISO_LIB}")
else()
    # Linux: need to link OpenMP, math, gfortran (Pardiso is Fortran underneath),
    # and pthread. Use the gomp OpenMP runtime by default; if your Pardiso build
    # requires Intel OpenMP, replace gomp with iomp5.
    find_package(OpenMP REQUIRED)
    find_package(Threads REQUIRED)

    target_link_libraries(pardiso_solver PRIVATE
        "${PARDISO_LIB}"
        OpenMP::OpenMP_CXX
        Threads::Threads
        m
        gfortran
    )

    # So the executable can find libpardiso.so at runtime without LD_LIBRARY_PATH
    set_target_properties(pardiso_solver PROPERTIES
        BUILD_RPATH   "${PARDISO_DIR}/lib"
        INSTALL_RPATH "${PARDISO_DIR}/lib"
    )
endif()

# =============================================================================
# Windows-only: copy DLLs next to the .exe so it runs without PATH setup
# =============================================================================
if(WIN32)
    add_custom_command(TARGET pardiso_solver POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PARDISO_DLL}"     $<TARGET_FILE_DIR:pardiso_solver>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${PARDISO_OMP_DLL}" $<TARGET_FILE_DIR:pardiso_solver>
        COMMENT "Copying Pardiso DLLs to output directory"
    )
endif()
