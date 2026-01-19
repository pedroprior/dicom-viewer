cmake_minimum_required(VERSION 3.20)
project(DicomViewer VERSION 1.0 LANGUAGES CXX)

# ==============================================================================
# Global configuration
# ==============================================================================
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(BUILD_SHARED_LIBS OFF)

# ==============================================================================
# Qt6
# ==============================================================================
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# ==============================================================================
# Platform-specific settings
# ==============================================================================
if(WIN32)
    # Windows-specific settings
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_definitions(-DNOMINMAX)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    
    # Use static runtime on Windows for easier distribution
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ==============================================================================
# DCMTK – build from third_party using ExternalProject
# ==============================================================================
include(ExternalProject)

set(DCMTK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dcmtk")
set(DCMTK_INSTALL_DIR "${CMAKE_CURRENT_BINARY_DIR}/dcmtk-install")

# Platform-specific library suffix
if(WIN32)
    set(LIB_SUFFIX ".lib")
    set(LIB_PREFIX "")
else()
    set(LIB_SUFFIX ".a")
    set(LIB_PREFIX "lib")
endif()

# Configure DCMTK build
set(DCMTK_CMAKE_ARGS
    -DCMAKE_BUILD_TYPE=$<CONFIG>
    -DCMAKE_INSTALL_PREFIX=${DCMTK_INSTALL_DIR}
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    # Core
    -DBUILD_SHARED_LIBS=OFF
    -DBUILD_APPS=OFF
    -DDCMTK_WITH_THREADS=ON
    -DDCMTK_ENABLE_BUILTIN_OFICONV_DATA=ON
    # Enable codec modules
    -DDCMTK_ENABLE_DCMJPEG=ON
    -DDCMTK_ENABLE_DCMJPLS=ON
    # Use built-in codecs
    -DDCMTK_WITH_CHARLS=OFF
    # Disable everything else
    -DDCMTK_WITH_OPENSSL=OFF
    -DDCMTK_WITH_PNG=OFF
    -DDCMTK_WITH_TIFF=OFF
    -DDCMTK_WITH_XML=OFF
    -DDCMTK_WITH_ZLIB=OFF
    -DDCMTK_WITH_SNDFILE=OFF
    -DDCMTK_WITH_ICONV=OFF
)

# Windows-specific DCMTK settings
if(WIN32)
    list(APPEND DCMTK_CMAKE_ARGS
        -DCMAKE_MSVC_RUNTIME_LIBRARY=${CMAKE_MSVC_RUNTIME_LIBRARY}
        -DDCMTK_OVERWRITE_WIN32_COMPILER_FLAGS=OFF
    )
endif()

ExternalProject_Add(dcmtk_external
    SOURCE_DIR ${DCMTK_SOURCE_DIR}
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/dcmtk-build
    CMAKE_ARGS ${DCMTK_CMAKE_ARGS}
    CMAKE_GENERATOR ${CMAKE_GENERATOR}
    BUILD_BYPRODUCTS
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}dcmdata${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}dcmimgle${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}dcmimage${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}dcmjpeg${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}dcmjpls${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}ofstd${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}oflog${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}oficonv${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}ijg8${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}ijg12${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}ijg16${LIB_SUFFIX}
        ${DCMTK_INSTALL_DIR}/lib/${LIB_PREFIX}dcmtkcharls${LIB_SUFFIX}
    INSTALL_DIR ${DCMTK_INSTALL_DIR}
)

# ==============================================================================
# Create imported targets for DCMTK libraries
# ==============================================================================
set(DCMTK_INCLUDE_DIRS "${DCMTK_INSTALL_DIR}/include")
set(DCMTK_LIBRARY_DIR "${DCMTK_INSTALL_DIR}/lib")

# Helper function to create imported library
function(add_dcmtk_library name)
    add_library(dcmtk::${name} STATIC IMPORTED)
    set_target_properties(dcmtk::${name} PROPERTIES
        IMPORTED_LOCATION "${DCMTK_LIBRARY_DIR}/${LIB_PREFIX}${name}${LIB_SUFFIX}"
    )
    add_dependencies(dcmtk::${name} dcmtk_external)
endfunction()

# Create imported targets
add_dcmtk_library(dcmdata)
add_dcmtk_library(dcmimgle)
add_dcmtk_library(dcmimage)
add_dcmtk_library(dcmjpeg)
add_dcmtk_library(dcmjpls)
add_dcmtk_library(ofstd)
add_dcmtk_library(oflog)
add_dcmtk_library(oficonv)
add_dcmtk_library(ijg8)
add_dcmtk_library(ijg12)
add_dcmtk_library(ijg16)
add_dcmtk_library(dcmtkcharls)

# ==============================================================================
# Executable
# ==============================================================================
add_executable(dicom_viewer
    src/main.cpp
    src/core/dicom_image.cpp
    src/core/dicom_metadata.cpp
    src/infrastructure/dcmtk_wrapper.cpp
    src/ui/main_window.cpp
)

# Ensure DCMTK is built before our target
add_dependencies(dicom_viewer dcmtk_external)

target_include_directories(dicom_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/infrastructure
    ${DCMTK_INCLUDE_DIRS}
)

# Order matters for static linking!
target_link_libraries(dicom_viewer PRIVATE
    Qt6::Widgets
    dcmtk::dcmimage
    dcmtk::dcmjpls
    dcmtk::dcmtkcharls
    dcmtk::dcmjpeg
    dcmtk::ijg8
    dcmtk::ijg12
    dcmtk::ijg16
    dcmtk::dcmimgle
    dcmtk::dcmdata
    dcmtk::oflog
    dcmtk::ofstd
    dcmtk::oficonv
)

# Platform-specific link libraries
if(WIN32)
    target_link_libraries(dicom_viewer PRIVATE
        ws2_32
        netapi32
        wsock32
        iphlpapi
    )
else()
    target_link_libraries(dicom_viewer PRIVATE
        pthread
        dl
    )
endif()

# ==============================================================================
# Warnings
# ==============================================================================
if(MSVC)
    target_compile_options(dicom_viewer PRIVATE /W4 /utf-8)
else()
    target_compile_options(dicom_viewer PRIVATE -Wall -Wextra -pedantic)
endif()

# ==============================================================================
# Install
# ==============================================================================
install(TARGETS dicom_viewer RUNTIME DESTINATION bin)

# Windows: Copy Qt DLLs
if(WIN32)
    # Use windeployqt to copy Qt dependencies
    find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
    if(WINDEPLOYQT)
        add_custom_command(TARGET dicom_viewer POST_BUILD
            COMMAND ${WINDEPLOYQT} --no-translations --no-system-d3d-compiler 
                    --no-opengl-sw $<TARGET_FILE:dicom_viewer>
            COMMENT "Running windeployqt..."
        )
    endif()
endif()

# ==============================================================================
# Summary
# ==============================================================================
message(STATUS "")
message(STATUS "========================================")
message(STATUS "DICOM Viewer Configuration Summary")
message(STATUS "========================================")
message(STATUS "  CMake version:    ${CMAKE_VERSION}")
message(STATUS "  Generator:        ${CMAKE_GENERATOR}")
message(STATUS "  Build type:       ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard:     ${CMAKE_CXX_STANDARD}")
message(STATUS "  Platform:         ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Qt6 version:      ${Qt6_VERSION}")
message(STATUS "  DCMTK source:     ${DCMTK_SOURCE_DIR}")
message(STATUS "  DCMTK install:    ${DCMTK_INSTALL_DIR}")
message(STATUS "========================================")
message(STATUS "")