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

    double wc = static_cast<double>(window_center);
    double ww = static_cast<double>(window_width);
    double pv = static_cast<double>(pixel_value);

    double lower = wc - (ww / 2.0);
    double upper = wc + (ww / 2.0);

    double output;
    if (pv <= lower) {
        output = 0.0;
    }
    else if (pv >= upper) {
        output = 255.0;
    }
    else {
        output = ((pv - lower) / ww) * 255.0;
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

    std::cout << "[DEBUG] Auto W/L - Data range: " << min_val << " - " << max_val << std::endl;

    if (max_val - min_val < 10) {
        data_.window_center = (static_cast<int32_t>(max_val) + static_cast<int32_t>(min_val)) / 2;
        data_.window_width = 256;
        std::cout << "[DEBUG] Auto W/L (small range) - WC: " << data_.window_center
            << ", WW: " << data_.window_width << std::endl;
        return;
    }

    const size_t num_bins = 4096;
    const double range = static_cast<double>(max_val - min_val + 1);
    const double bin_size = range / static_cast<double>(num_bins);
    std::vector<uint32_t> histogram(num_bins, 0);

    for (uint16_t pixel : data_.pixels) {
        size_t bin = static_cast<size_t>((pixel - min_val) / bin_size);
        if (bin >= num_bins) bin = num_bins - 1;
        histogram[bin]++;
    }

    const size_t total_pixels = data_.pixels.size();

    const size_t lower_threshold = total_pixels / 100;
    const size_t upper_threshold = total_pixels / 100;

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
    for (int i = static_cast<int>(num_bins) - 1; i >= 0; --i) {
        cumulative += histogram[static_cast<size_t>(i)];
        if (cumulative > upper_threshold) {
            upper_bin = static_cast<size_t>(i);
            break;
        }
    }

    int32_t lower_bound = static_cast<int32_t>(min_val + lower_bin * bin_size);
    int32_t upper_bound = static_cast<int32_t>(min_val + (upper_bin + 1) * bin_size);

    lower_bound = std::max(lower_bound, static_cast<int32_t>(min_val));
    upper_bound = std::min(upper_bound, static_cast<int32_t>(max_val));

    data_.window_center = (upper_bound + lower_bound) / 2;
    data_.window_width = upper_bound - lower_bound;

    if (data_.window_width < 100) {
        data_.window_width = 100;
    }

    std::cout << "[DEBUG] Auto W/L - Effective range: " << lower_bound << " - " << upper_bound << std::endl;
    std::cout << "[DEBUG] Auto W/L - WC: " << data_.window_center
        << ", WW: " << data_.window_width << std::endl;
}

std::vector<uint8_t> DicomImageData::to_display_buffer(
    int32_t window_center,
    int32_t window_width
) const {
    if (data_.is_rgb()) {
        return to_rgb_display_buffer();
    }

    if (data_.is_preprocessed && !data_.processed_pixels.empty()) {
        return data_.processed_pixels;
    }

    const size_t pixel_count = data_.width * data_.height;
    std::vector<uint8_t> display_buffer(pixel_count);

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
        const size_t pixel_count = data_.width * data_.height;
        std::vector<uint8_t> rgb_buffer(pixel_count * 3);

        auto gray_buffer = to_display_buffer(data_.window_center, data_.window_width);

        for (size_t i = 0; i < pixel_count; ++i) {
            uint8_t gray = gray_buffer[i];
            rgb_buffer[i * 3 + 0] = gray;
            rgb_buffer[i * 3 + 1] = gray;
            rgb_buffer[i * 3 + 2] = gray;
        }

        return rgb_buffer;
    }

    return data_.rgb_pixels;
}