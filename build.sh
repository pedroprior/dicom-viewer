#!/bin/bash

# DICOM Viewer Build Script for Linux/macOS
# This script handles the complete build process including DCMTK setup

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Configuration
BUILD_TYPE="${1:-Release}"  # Default to Release, can pass Debug as argument
NUM_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
DCMTK_VERSION="DCMTK-3.6.8"

# Script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd "$SCRIPT_DIR"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}DICOM Viewer Build Script${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""

# Function to print status messages
print_status() {
    echo -e "${GREEN}[*]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

# Check for required tools
print_status "Checking dependencies..."

command -v git >/dev/null 2>&1 || {
    print_error "git is required but not installed. Aborting."
    exit 1
}

command -v cmake >/dev/null 2>&1 || {
    print_error "cmake is required but not installed. Aborting."
    exit 1
}

command -v make >/dev/null 2>&1 || command -v ninja >/dev/null 2>&1 || {
    print_error "make or ninja is required but not installed. Aborting."
    exit 1
}

# Check for C++ compiler
if command -v g++ >/dev/null 2>&1; then
    print_status "Found g++ compiler"
elif command -v clang++ >/dev/null 2>&1; then
    print_status "Found clang++ compiler"
else
    print_error "No C++ compiler found (g++ or clang++). Aborting."
    exit 1
fi

# Check for Qt6
print_status "Checking for Qt6..."
if ! command -v qmake6 >/dev/null 2>&1 && ! command -v qmake >/dev/null 2>&1; then
    print_warning "Qt6 not found in PATH. You may need to set CMAKE_PREFIX_PATH."
    echo "Example: export CMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64"
fi

print_status "All required dependencies found!"
echo ""

# Setup DCMTK
print_status "Setting up DCMTK..."

if [ ! -d "third_party" ]; then
    print_status "Creating third_party directory..."
    mkdir -p third_party
fi

if [ ! -d "third_party/dcmtk" ]; then
    print_status "Cloning DCMTK repository..."
    cd third_party
    git clone --depth 1 --branch ${DCMTK_VERSION} https://github.com/DCMTK/dcmtk.git
    cd ..
    print_status "DCMTK cloned successfully!"
else
    print_status "DCMTK already exists in third_party/"
fi

# Check if DCMTK needs to be built
if [ ! -f "third_party/dcmtk/install/lib/libofstd.a" ]; then
    print_status "DCMTK will be built during CMake configuration (this may take 10-15 minutes)..."
    print_warning "This is a one-time build. Subsequent builds will be much faster."
else
    print_status "DCMTK already built"
fi

echo ""

# Create build directory
print_status "Creating build directory..."
BUILD_DIR="build-${BUILD_TYPE}"

if [ -d "$BUILD_DIR" ]; then
    print_warning "Build directory exists. Cleaning..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

print_status "Build directory: $BUILD_DIR"
print_status "Build type: $BUILD_TYPE"
print_status "Using $NUM_JOBS parallel jobs"
echo ""

# Configure
print_status "Configuring CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=${BUILD_TYPE} \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    || {
        print_error "CMake configuration failed!"
        exit 1
    }

echo ""

# Build
print_status "Building DICOM Viewer..."
cmake --build . --config ${BUILD_TYPE} -j${NUM_JOBS} || {
    print_error "Build failed!"
    exit 1
}

echo ""
print_status "Build completed successfully!"
echo ""

# Print binary location
if [ -f "dicom_viewer" ]; then
    BINARY_PATH="$(pwd)/dicom_viewer"
    print_status "Executable location: ${BINARY_PATH}"
    
    # Make executable
    chmod +x dicom_viewer
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Build Summary${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo -e "Build Type:   ${BUILD_TYPE}"
    echo -e "Binary:       ${BINARY_PATH}"
    echo ""
    echo -e "To run the application:"
    echo -e "  ${BLUE}cd ${BUILD_DIR}${NC}"
    echo -e "  ${BLUE}./dicom_viewer${NC}"
    echo ""
    echo -e "Or directly:"
    echo -e "  ${BLUE}${BINARY_PATH}${NC}"
    echo -e "${GREEN}========================================${NC}"
    
elif [ -f "${BUILD_TYPE}/dicom_viewer" ]; then
    BINARY_PATH="$(pwd)/${BUILD_TYPE}/dicom_viewer"
    print_status "Executable location: ${BINARY_PATH}"
    chmod +x "${BUILD_TYPE}/dicom_viewer"
    
    echo ""
    echo -e "${GREEN}========================================${NC}"
    echo -e "${GREEN}Build Summary${NC}"
    echo -e "${GREEN}========================================${NC}"
    echo -e "Build Type:   ${BUILD_TYPE}"
    echo -e "Binary:       ${BINARY_PATH}"
    echo ""
    echo -e "To run the application:"
    echo -e "  ${BLUE}${BINARY_PATH}${NC}"
    echo -e "${GREEN}========================================${NC}"
else
    print_warning "Could not find executable. Check build output above."
fi

echo ""