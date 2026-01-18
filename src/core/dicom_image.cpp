#include "dicom_image.hpp"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iostream>

uint8_t DicomImageData::apply_window_level(
    uint16_t pixel_value,
    int32_t window_center,
    int32_t window_width,
    bool invert
) {
    if (window_width < 1) {
        window_width = 1;
    }
    
    double lower = window_center - (window_width / 2.0);
    double upper = window_center + (window_width / 2.0);
    
    double output;
    if (pixel_value <= lower) {
        output = 0.0;
    } else if (pixel_value >= upper) {
        output = 255.0;
    } else {
        output = ((pixel_value - lower) / window_width) * 255.0;
    }
    
    if (invert) {
        output = 255.0 - output;
    }
    
    return static_cast<uint8_t>(std::clamp(output, 0.0, 255.0));
}

void DicomImageData::auto_window_level() {
    if (data_.pixels.empty()) {
        return;
    }
    
    uint16_t min_val = *std::min_element(data_.pixels.begin(), data_.pixels.end());
    uint16_t max_val = *std::max_element(data_.pixels.begin(), data_.pixels.end());
    
    if (max_val - min_val < 10) {
        data_.window_center = (max_val + min_val) / 2;
        data_.window_width = 256;
        return;
    }
    
    // Histograma para encontrar range efetivo (excluir outliers)
    const size_t num_bins = 256;
    const double bin_size = (max_val - min_val + 1) / static_cast<double>(num_bins);
    std::vector<uint32_t> histogram(num_bins, 0);
    
    for (uint16_t pixel : data_.pixels) {
        size_t bin = static_cast<size_t>((pixel - min_val) / bin_size);
        if (bin >= num_bins) bin = num_bins - 1;
        histogram[bin]++;
    }
    
    const size_t total_pixels = data_.pixels.size();
    const size_t lower_threshold = total_pixels * 5 / 1000;   // 0.5%
    const size_t upper_threshold = total_pixels * 5 / 1000;   // 0.5%
    
    size_t cumulative = 0;
    size_t lower_bin = 0;
    for (size_t i = 0; i < num_bins; ++i) {
        cumulative += histogram[i];
        if (cumulative > lower_threshold) {
            lower_bin = i;
            break;
        }
    }
    
    cumulative = 0;
    size_t upper_bin = num_bins - 1;
    for (int i = num_bins - 1; i >= 0; --i) {
        cumulative += histogram[i];
        if (cumulative > upper_threshold) {
            upper_bin = i;
            break;
        }
    }
    
    uint16_t lower_bound = min_val + static_cast<uint16_t>(lower_bin * bin_size);
    uint16_t upper_bound = min_val + static_cast<uint16_t>((upper_bin + 1) * bin_size);
    
    data_.window_center = (upper_bound + lower_bound) / 2;
    data_.window_width = upper_bound - lower_bound;
    
    if (data_.window_width < 100) {
        data_.window_width = 100;
    }
    
    std::cout << "[DEBUG] Auto window: range " << lower_bound << "-" << upper_bound
              << " -> WC=" << data_.window_center << ", WW=" << data_.window_width << std::endl;
}

std::vector<uint8_t> DicomImageData::to_display_buffer(
    int32_t window_center,
    int32_t window_width
) const {
    if (data_.is_rgb()) {
        return to_rgb_display_buffer();
    }
    
    // Se temos dados pré-processados, usar diretamente
    if (data_.is_preprocessed && !data_.processed_pixels.empty()) {
        return data_.processed_pixels;
    }
    
    // Aplicar window/level aos dados raw
    const size_t pixel_count = data_.width * data_.height;
    std::vector<uint8_t> display_buffer(pixel_count);
    
    // Não precisa mais inverter - já foi feito no carregamento
    bool invert = false;
    
    for (size_t i = 0; i < pixel_count; ++i) {
        display_buffer[i] = apply_window_level(
            data_.pixels[i],
            window_center,
            window_width,
            invert
        );
    }
    
    return display_buffer;
}

std::vector<uint8_t> DicomImageData::to_rgb_display_buffer() const {
    if (!data_.is_rgb()) {
        // Grayscale to RGB conversion
        const size_t pixel_count = data_.width * data_.height;
        std::vector<uint8_t> rgb_buffer(pixel_count * 3);
        
        auto gray_buffer = to_display_buffer(data_.window_center, data_.window_width);
        
        for (size_t i = 0; i < pixel_count; ++i) {
            uint8_t gray = gray_buffer[i];
            rgb_buffer[i * 3 + 0] = gray;  // R
            rgb_buffer[i * 3 + 1] = gray;  // G
            rgb_buffer[i * 3 + 2] = gray;  // B
        }
        
        return rgb_buffer;
    }
    
    // Already RGB, just return a copy
    return data_.rgb_pixels;
}
