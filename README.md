# DICOM Viewer

A modern C++20 DICOM medical image viewer with comprehensive metadata display and window/level controls.

## Features

### Core Functionality
- ✅ Load and display DICOM (.dcm) files via file dialog
- ✅ Support for grayscale images (MONOCHROME1, MONOCHROME2)
- ✅ Support for RGB color images
- ✅ Accurate image rendering with proper orientation
- ✅ Error handling with informative messages

### Advanced Features
- 🎚️ **Window/Level Controls**: Interactive sliders and spinboxes for adjusting image contrast
- 🔄 **Auto Window/Level**: Automatically calculate optimal display settings
- 📊 **Metadata Display**: Comprehensive DICOM tag information including:
  - Patient information (name, ID, age, sex, birth date)
  - Study details (date, time, description, accession number)
  - Series information (modality, series number, description)
  - Equipment data (manufacturer, model, institution)
  - Image characteristics (dimensions, bit depth, photometric interpretation)
  - Window/level settings

## Architecture

The application follows clean architecture principles with clear separation of concerns:

```
dicom-viewer/
├── src/
│   ├── core/                    # Domain logic (no external dependencies)
│   │   ├── result.hpp           # Error handling without exceptions
│   │   ├── error_codes.hpp      # Error definitions
│   │   ├── dicom_image.hpp/cpp  # Image data and processing
│   │   └── dicom_metadata.hpp/cpp
│   ├── infrastructure/          # External library wrappers
│   │   └── dcmtk_wrapper.hpp/cpp # DCMTK abstraction layer
│   └── ui/                      # Qt user interface
│       └── main_window.hpp/cpp
└── CMakeLists.txt
```

### Design Principles

1. **No Exceptions**: Uses `Result<T, E>` type for explicit error handling
2. **Wrapper Pattern**: DCMTK library completely isolated behind clean interface
3. **PIMPL Idiom**: DCMTK headers hidden in implementation files
4. **Modern C++**: Uses C++20 features, RAII, smart pointers
5. **Testability**: Interface-based design allows easy mocking

## Libraries Used

- **DCMTK 3.6.7+**: DICOM file parsing and image decoding
  - `dcmdata`: DICOM data structures
  - `dcmimgle`: Image processing for grayscale
  - `dcmimage`: Image processing for color
- **Qt 6.x**: Cross-platform GUI framework
  - Widgets module for UI components
  - Image handling for display

## Building

### Prerequisites

**Windows:**
```powershell
# Install Qt6
winget install Qt.Qt.6

# Install DCMTK (via vcpkg)
vcpkg install dcmtk:x64-windows
```

**Linux (Ubuntu/Debian):**
```bash
sudo apt update
sudo apt install qt6-base-dev libdcmtk-dev cmake build-essential
```

**macOS:**
```bash
brew install qt@6 dcmtk cmake
```

### Compilation

```bash
# Clone the repository
git clone <repository-url>
cd dicom-viewer

# Create build directory
mkdir build
cd build

# Configure
cmake ..

# Build
cmake --build . --config Release

# Run
./dicom_viewer
```

# Building using scripts
./build.sh (Unix) or ./build.bat (Windows)


### Build Options

```bash
# Disable exceptions (optional)
cmake -DDISABLE_EXCEPTIONS=ON ..

# Specify Qt path (if not auto-detected)
cmake -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64 ..

# Specify DCMTK path (if not auto-detected)
cmake -DDCMTK_DIR=/path/to/dcmtk/cmake ..
```

## Usage

1. **Open DICOM File**: `File > Open` or click "Open" toolbar button
2. **Adjust Window/Level**:
   - Use sliders or spinboxes to manually adjust
   - Click "Auto" for automatic optimal settings
   - Click "Reset" to restore original values
3. **View Metadata**: Metadata panel shows all DICOM tags
4. **Toggle Metadata**: `View > Toggle Metadata Panel` or press `M`

### Keyboard Shortcuts

- `Ctrl+O` / `Cmd+O`: Open file
- `Ctrl+Q` / `Cmd+Q`: Quit application
- `M`: Toggle metadata panel

## Window/Level Implementation

The viewer implements standard DICOM windowing according to DICOM Part 3:

```
if (pixel_value <= window_center - window_width/2)
    output = 0
else if (pixel_value >= window_center + window_width/2)
    output = 255
else
    output = ((pixel_value - lower) / window_width) * 255
```

- **MONOCHROME2**: Lower values = darker (standard X-ray)
- **MONOCHROME1**: Lower values = brighter (inverted)
- **Auto Window/Level**: Uses histogram analysis to find optimal range

## RGB Image Support

The viewer supports RGB DICOM images with:
- Automatic detection of RGB photometric interpretation
- Direct display without window/level adjustment
- 8-bit per channel output

## Error Handling

All errors are handled without exceptions using the `Result<T, E>` pattern:

```cpp
auto result = dicom_reader->load_image(path);
if (result.is_error()) {
    // Handle error
    display_error(result.error());
    return;
}
auto image = std::move(result.value());
```

Error types include:
- File not found
- Invalid DICOM format
- Unsupported transfer syntax
- Missing pixel data
- Invalid image dimensions
- Unsupported photometric interpretation

## Testing

To test with sample DICOM files:
1. Download test images from:
   - [DICOM Library](https://www.dicomlibrary.com/)
   - [OsiriX Sample Data](https://www.osirix-viewer.com/resources/dicom-image-library/)

2. Test different scenarios:
   - Grayscale CT/MRI scans
   - RGB color images
   - Various window/level presets
   - Different bit depths (8, 12, 16-bit)

## Known Limitations

- No support for multi-frame DICOM images
- No DICOM network (PACS) support
- Limited to uncompressed or basic compressed transfer syntaxes
- No image manipulation tools (zoom, pan, rotate)

## License
This project is developed as a technical assessment and is provided as-is for evaluation purposes.

## Technical Details

### Memory Management
- Uses RAII throughout
- Smart pointers for resource management
- No manual memory allocation in application code
- DCMTK memory handled in wrapper layer

### Thread Safety
- Single-threaded design (Qt main thread)
- All DCMTK operations on main thread
- No concurrent access to shared state

### Performance
- Lazy pixel data conversion
- Caching of display buffers
- Efficient window/level calculation
- Minimal copies using move semantics

## Contact
For questions or issues, please contact pedropriordev@gmail.com