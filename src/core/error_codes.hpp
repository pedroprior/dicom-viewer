#pragma once

#include <string>
#include <string_view>

enum class DicomError {
    FileNotFound,
    InvalidFormat,
    UnsupportedTransferSyntax,
    MissingPixelData,
    InvalidImageDimensions,
    MemoryAllocationFailed,
    UnsupportedPhotometricInterpretation,
    InvalidMetadata,
    UnknownError
};

struct ErrorInfo {
    DicomError code;
    std::string message;
    std::string details;
    
    ErrorInfo(DicomError c, std::string msg, std::string det = "")
        : code(c), message(std::move(msg)), details(std::move(det)) {}
    
    std::string full_message() const {
        if (details.empty()) {
            return message;
        }
        return message + ": " + details;
    }
    
    static constexpr std::string_view error_name(DicomError code) {
        switch (code) {
            case DicomError::FileNotFound: return "FileNotFound";
            case DicomError::InvalidFormat: return "InvalidFormat";
            case DicomError::UnsupportedTransferSyntax: return "UnsupportedTransferSyntax";
            case DicomError::MissingPixelData: return "MissingPixelData";
            case DicomError::InvalidImageDimensions: return "InvalidImageDimensions";
            case DicomError::MemoryAllocationFailed: return "MemoryAllocationFailed";
            case DicomError::UnsupportedPhotometricInterpretation: return "UnsupportedPhotometricInterpretation";
            case DicomError::InvalidMetadata: return "InvalidMetadata";
            default: return "UnknownError";
        }
    }
};