#pragma once

#include <string>
#include <optional>
#include <cstdint>

struct DicomMetadata {
    // Patient Information
    std::optional<std::string> patient_name;
    std::optional<std::string> patient_id;
    std::optional<std::string> patient_birth_date;
    std::optional<std::string> patient_sex;
    std::optional<std::string> patient_age;
    
    // Study Information
    std::optional<std::string> study_date;
    std::optional<std::string> study_time;
    std::optional<std::string> study_description;
    std::optional<std::string> study_instance_uid;
    std::optional<std::string> accession_number;
    
    // Series Information
    std::optional<std::string> series_date;
    std::optional<std::string> series_time;
    std::optional<std::string> series_description;
    std::optional<std::string> series_instance_uid;
    std::optional<std::string> series_number;
    std::optional<std::string> modality;
    
    // Image Information
    std::optional<std::string> instance_number;
    std::optional<std::string> image_type;
    std::optional<std::string> sop_class_uid;
    std::optional<std::string> sop_instance_uid;
    
    // Equipment Information
    std::optional<std::string> manufacturer;
    std::optional<std::string> manufacturer_model_name;
    std::optional<std::string> station_name;
    std::optional<std::string> institution_name;
    
    // Image Characteristics
    std::optional<uint32_t> rows;
    std::optional<uint32_t> columns;
    std::optional<uint16_t> bits_allocated;
    std::optional<uint16_t> bits_stored;
    std::optional<uint16_t> high_bit;
    std::optional<uint16_t> samples_per_pixel;
    std::optional<std::string> photometric_interpretation;
    std::optional<std::string> pixel_spacing;
    std::optional<double> slice_thickness;
    
    // Window/Level
    std::optional<int32_t> window_center;
    std::optional<int32_t> window_width;
    std::optional<std::string> window_explanation;
    
    // Transfer Syntax
    std::optional<std::string> transfer_syntax_uid;
    
    // Helper method to format for display
    std::string to_string() const;
};