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
# DCMTK – build from third_party
# ==============================================================================
set(DCMTK_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party/dcmtk")
set(DCMTK_BUILD_DIR "${DCMTK_SOURCE_DIR}/build")
set(DCMTK_INSTALL_DIR "${DCMTK_SOURCE_DIR}/install")
file(REMOVE_RECURSE "${DCMTK_BUILD_DIR}" "${DCMTK_INSTALL_DIR}")
file(MAKE_DIRECTORY "${DCMTK_BUILD_DIR}")
message(STATUS "Building DCMTK from source...")
execute_process(
    COMMAND ${CMAKE_COMMAND}
        -DCMAKE_BUILD_TYPE=Release
        -DCMAKE_INSTALL_PREFIX=${DCMTK_INSTALL_DIR}
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
        ${DCMTK_SOURCE_DIR}
    WORKING_DIRECTORY "${DCMTK_BUILD_DIR}"
    RESULT_VARIABLE DCMTK_CONFIGURE_RESULT
)
if(NOT DCMTK_CONFIGURE_RESULT EQUAL 0)
    message(FATAL_ERROR "DCMTK configuration failed")
endif()
execute_process(
    COMMAND ${CMAKE_COMMAND} --build . --parallel
    WORKING_DIRECTORY "${DCMTK_BUILD_DIR}"
    RESULT_VARIABLE DCMTK_BUILD_RESULT
)
if(NOT DCMTK_BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "DCMTK build failed")
endif()
execute_process(
    COMMAND ${CMAKE_COMMAND} --install .
    WORKING_DIRECTORY "${DCMTK_BUILD_DIR}"
    RESULT_VARIABLE DCMTK_INSTALL_RESULT
)
if(NOT DCMTK_INSTALL_RESULT EQUAL 0)
    message(FATAL_ERROR "DCMTK install failed")
endif()
message(STATUS "DCMTK built successfully")
# ==============================================================================
# DCMTK include paths and libraries
# ==============================================================================
set(DCMTK_INCLUDE_DIRS "${DCMTK_INSTALL_DIR}/include")
set(DCMTK_LIBRARY_DIR "${DCMTK_INSTALL_DIR}/lib")
find_library(DCMTK_dcmdata_LIBRARY dcmdata PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_dcmimgle_LIBRARY dcmimgle PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_dcmimage_LIBRARY dcmimage PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_dcmjpeg_LIBRARY dcmjpeg PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_dcmjpls_LIBRARY dcmjpls PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_ofstd_LIBRARY ofstd PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_oflog_LIBRARY oflog PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_oficonv_LIBRARY oficonv PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
# Internal JPEG libs
find_library(DCMTK_ijg8_LIBRARY ijg8 PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_ijg12_LIBRARY ijg12 PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
find_library(DCMTK_ijg16_LIBRARY ijg16 PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
# Internal CharLS lib (name varies by DCMTK version)
find_library(DCMTK_charls_LIBRARY NAMES dcmtkcharls charls PATHS ${DCMTK_LIBRARY_DIR} NO_DEFAULT_PATH REQUIRED)
set(DCMTK_LIBRARIES
    ${DCMTK_dcmimage_LIBRARY}
    ${DCMTK_dcmjpls_LIBRARY}
    ${DCMTK_charls_LIBRARY}  # After dcmjpls
    ${DCMTK_dcmjpeg_LIBRARY}
    ${DCMTK_ijg8_LIBRARY}
    ${DCMTK_ijg12_LIBRARY}
    ${DCMTK_ijg16_LIBRARY}  # After dcmjpeg
    ${DCMTK_dcmimgle_LIBRARY}
    ${DCMTK_dcmdata_LIBRARY}
    ${DCMTK_oflog_LIBRARY}
    ${DCMTK_ofstd_LIBRARY}
    ${DCMTK_oficonv_LIBRARY}
)
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
target_include_directories(dicom_viewer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/infrastructure
    ${DCMTK_INCLUDE_DIRS}
)
target_link_libraries(dicom_viewer PRIVATE
    Qt6::Widgets
    ${DCMTK_LIBRARIES}
)
# ==============================================================================
# Warnings
# ==============================================================================
if(MSVC)
    target_compile_options(dicom_viewer PRIVATE /W4)
else()
    target_compile_options(dicom_viewer PRIVATE -Wall -Wextra -pedantic)
endif()
install(TARGETS dicom_viewer RUNTIME DESTINATION bin)