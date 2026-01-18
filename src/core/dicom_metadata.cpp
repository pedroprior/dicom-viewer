#include "dicom_metadata.hpp"
#include <sstream>

std::string DicomMetadata::to_string() const {
    std::ostringstream oss;
    
    oss << "=== DICOM Metadata ===" << std::endl;
    
    // Patient Information
    if (patient_name || patient_id || patient_birth_date || patient_sex) {
        oss << "\n[Patient Information]" << std::endl;
        if (patient_name) oss << "  Name: " << *patient_name << std::endl;
        if (patient_id) oss << "  ID: " << *patient_id << std::endl;
        if (patient_birth_date) oss << "  Birth Date: " << *patient_birth_date << std::endl;
        if (patient_sex) oss << "  Sex: " << *patient_sex << std::endl;
        if (patient_age) oss << "  Age: " << *patient_age << std::endl;
    }
    
    // Study Information
    if (study_date || study_description || accession_number) {
        oss << "\n[Study Information]" << std::endl;
        if (study_date) oss << "  Date: " << *study_date << std::endl;
        if (study_time) oss << "  Time: " << *study_time << std::endl;
        if (study_description) oss << "  Description: " << *study_description << std::endl;
        if (accession_number) oss << "  Accession Number: " << *accession_number << std::endl;
        if (study_instance_uid) oss << "  Study UID: " << *study_instance_uid << std::endl;
    }
    
    // Series Information
    if (modality || series_description || series_number) {
        oss << "\n[Series Information]" << std::endl;
        if (modality) oss << "  Modality: " << *modality << std::endl;
        if (series_number) oss << "  Series Number: " << *series_number << std::endl;
        if (series_description) oss << "  Description: " << *series_description << std::endl;
        if (series_date) oss << "  Date: " << *series_date << std::endl;
        if (series_instance_uid) oss << "  Series UID: " << *series_instance_uid << std::endl;
    }
    
    // Equipment Information
    if (manufacturer || manufacturer_model_name || institution_name) {
        oss << "\n[Equipment Information]" << std::endl;
        if (manufacturer) oss << "  Manufacturer: " << *manufacturer << std::endl;
        if (manufacturer_model_name) oss << "  Model: " << *manufacturer_model_name << std::endl;
        if (station_name) oss << "  Station: " << *station_name << std::endl;
        if (institution_name) oss << "  Institution: " << *institution_name << std::endl;
    }
    
    // Image Characteristics
    if (rows || columns || bits_allocated) {
        oss << "\n[Image Characteristics]" << std::endl;
        if (rows && columns) {
            oss << "  Dimensions: " << *columns << " x " << *rows << std::endl;
        }
        if (samples_per_pixel) oss << "  Samples Per Pixel: " << *samples_per_pixel << std::endl;
        if (bits_allocated) oss << "  Bits Allocated: " << *bits_allocated << std::endl;
        if (bits_stored) oss << "  Bits Stored: " << *bits_stored << std::endl;
        if (photometric_interpretation) oss << "  Photometric: " << *photometric_interpretation << std::endl;
        if (pixel_spacing) oss << "  Pixel Spacing: " << *pixel_spacing << std::endl;
        if (slice_thickness) oss << "  Slice Thickness: " << *slice_thickness << " mm" << std::endl;
    }
    
    // Window/Level
    if (window_center || window_width) {
        oss << "\n[Window/Level]" << std::endl;
        if (window_center) oss << "  Window Center: " << *window_center << std::endl;
        if (window_width) oss << "  Window Width: " << *window_width << std::endl;
        if (window_explanation) oss << "  Explanation: " << *window_explanation << std::endl;
    }
    
    return oss.str();
}