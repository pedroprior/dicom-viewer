#pragma once

#include <vector>
#include <cstdint>
#include <optional>
#include <algorithm>

enum class PhotometricInterpretation {
    Monochrome1,  // Min value = white
    Monochrome2,  // Min value = black
    RGB,
    PaletteColor,
    Unknown
};

struct ImageData {
    std::vector<uint16_t> pixels;  // Raw pixel data (16-bit for grayscale)
    std::vector<uint8_t> rgb_pixels;  // RGB pixel data (8-bit per channel)
    std::vector<uint8_t> processed_pixels;  // Pre-processed 8-bit grayscale from DCMTK
    
    uint32_t width;
    uint32_t height;
    uint16_t bits_stored;
    uint16_t bits_allocated;
    uint16_t samples_per_pixel;  // 1 for grayscale, 3 for RGB
    bool is_signed;
    bool is_preprocessed;  // True if processed_pixels contains ready-to-display data
    
    PhotometricInterpretation photometric;
    
    // Window/Level for display (default values)
    int32_t window_center;
    int32_t window_width;
    
    ImageData() 
        : width(0), height(0), bits_stored(0), bits_allocated(0),
          samples_per_pixel(1), is_signed(false), is_preprocessed(false),
          photometric(PhotometricInterpretation::Monochrome2),
          window_center(0), window_width(0) {}
    
    bool is_rgb() const {
        return photometric == PhotometricInterpretation::RGB;
    }
    
    bool is_grayscale() const {
        return photometric == PhotometricInterpretation::Monochrome1 ||
               photometric == PhotometricInterpretation::Monochrome2;
    }
};

class DicomImageData {
    ImageData data_;
    
public:
    DicomImageData() = default;
    
    void set_data(ImageData data) {
        data_ = std::move(data);
    }
    
    const ImageData& data() const {
        return data_;
    }
    
    ImageData& data() {
        return data_;
    }
    
    // Convert grayscale to 8-bit display buffer with window/level
    std::vector<uint8_t> to_display_buffer(
        int32_t window_center,
        int32_t window_width
    ) const;
    
    // Convert RGB to 8-bit display buffer
    std::vector<uint8_t> to_rgb_display_buffer() const;
    
    // Auto-calculate optimal window/level from histogram
    void auto_window_level();
    
private:
    // Apply window/level to single pixel value
    static uint8_t apply_window_level(
        uint16_t pixel_value,
        int32_t window_center,
        int32_t window_width,
        bool invert
    );
};