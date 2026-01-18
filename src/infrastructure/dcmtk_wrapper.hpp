#pragma once

#include "core/result.hpp"
#include "core/error_codes.hpp"
#include "core/dicom_image.hpp"
#include "core/dicom_metadata.hpp"
#include <filesystem>
#include <memory>

class IDicomReader {
public:
    virtual ~IDicomReader() = default;
    
    virtual Result<DicomImageData, ErrorInfo> 
        load_image(const std::filesystem::path& path) = 0;
    
    virtual Result<DicomMetadata, ErrorInfo>
        load_metadata(const std::filesystem::path& path) = 0;
    
    // Load both image and metadata together
    virtual Result<std::pair<DicomImageData, DicomMetadata>, ErrorInfo>
        load_complete(const std::filesystem::path& path) = 0;
};

class DcmtkReader : public IDicomReader {
    // PIMPL pattern to hide DCMTK headers
    class Impl;
    std::unique_ptr<Impl> impl_;
    
public:
    DcmtkReader() noexcept;
    ~DcmtkReader() noexcept;
    
    // Delete copy, allow move
    DcmtkReader(const DcmtkReader&) = delete;
    DcmtkReader& operator=(const DcmtkReader&) = delete;
    DcmtkReader(DcmtkReader&&) noexcept;
    DcmtkReader& operator=(DcmtkReader&&) noexcept;
    
    Result<DicomImageData, ErrorInfo> 
        load_image(const std::filesystem::path& path) override;
    
    Result<DicomMetadata, ErrorInfo>
        load_metadata(const std::filesystem::path& path) override;
    
    Result<std::pair<DicomImageData, DicomMetadata>, ErrorInfo>
        load_complete(const std::filesystem::path& path) override;
};