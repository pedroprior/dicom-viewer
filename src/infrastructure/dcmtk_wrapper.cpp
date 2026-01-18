#include "dcmtk_wrapper.hpp"

// DCMTK includes
#include <dcmtk/dcmdata/dcrledrg.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmimage/diregist.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmimgle/diutils.h>
#include <dcmtk/dcmjpeg/dipijpeg.h>
#include <dcmtk/dcmjpeg/djdecode.h>
#include <dcmtk/dcmjpls/djdecode.h>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

class DcmtkReader::Impl {
public:
  Impl() {
    DcmRLEDecoderRegistration::registerCodecs();
    DJDecoderRegistration::registerCodecs(EDC_photometricInterpretation,
                                          EUC_default, EPC_default, OFTrue);
    DJLSDecoderRegistration::registerCodecs();
  }

  ~Impl() {
    DJLSDecoderRegistration::cleanup();
    DJDecoderRegistration::cleanup();
    DcmRLEDecoderRegistration::cleanup();
  }

  Result<DicomImageData, ErrorInfo>
  load_image_impl(const std::filesystem::path &path) noexcept {
    DicomImageData di_image;
    OFCondition status;

    DcmFileFormat file_format;
    status = file_format.loadFile(path.string().c_str());

    if (status.bad()) {
      return ErrorInfo{DicomError::InvalidFormat, "Failed to load DICOM file",
                       status.text()};
    }

    DcmDataset *dataset = file_format.getDataset();
    if (!dataset) {
      return ErrorInfo{DicomError::InvalidFormat,
                       "No dataset found in DICOM file", ""};
    }

    OFString photometric_str;
    dataset->findAndGetOFString(DCM_PhotometricInterpretation, photometric_str);

    PhotometricInterpretation photometric = PhotometricInterpretation::Unknown;
    if (photometric_str == "MONOCHROME1") {
      photometric = PhotometricInterpretation::Monochrome1;
    } else if (photometric_str == "MONOCHROME2") {
      photometric = PhotometricInterpretation::Monochrome2;
    } else if (photometric_str == "RGB") {
      photometric = PhotometricInterpretation::RGB;
    }

    if (photometric == PhotometricInterpretation::RGB) {
      auto result = load_rgb_image(dataset, file_format);
      if (result.is_error()) {
        return result.error();
      }
      di_image = std::move(result.value());
    } else if (photometric == PhotometricInterpretation::Monochrome1 ||
               photometric == PhotometricInterpretation::Monochrome2) {
      auto result = load_grayscale_image(dataset, file_format);
      if (result.is_error()) {
        return result.error();
      }
      di_image = std::move(result.value());
    } else {
      return ErrorInfo{DicomError::UnsupportedPhotometricInterpretation,
                       "Unsupported photometric interpretation",
                       photometric_str.c_str()};
    }

    return di_image;
  }

  Result<DicomMetadata, ErrorInfo>
  load_metadata_impl(const std::filesystem::path &path) noexcept {
    DcmFileFormat file_format;
    OFCondition status = file_format.loadFile(path.string().c_str());

    if (status.bad()) {
      return ErrorInfo{DicomError::InvalidFormat, "Failed to load DICOM file",
                       status.text()};
    }

    DcmDataset *dataset = file_format.getDataset();
    if (!dataset) {
      return ErrorInfo{DicomError::InvalidMetadata, "No dataset found", ""};
    }

    return extract_metadata(dataset);
  }

private:
  Result<DicomImageData, ErrorInfo>
  load_grayscale_image(DcmDataset *dataset,
                       DcmFileFormat &file_format) noexcept {
    
    OFString photometric_str;
    dataset->findAndGetOFString(DCM_PhotometricInterpretation, photometric_str);
    bool is_monochrome1 = (photometric_str == "MONOCHROME1");
    
    std::cout << "[DEBUG] Photometric from file: " << photometric_str.c_str() << std::endl;
    
    DcmXfer original_xfer(dataset->getOriginalXfer());
    std::cout << "[DEBUG] Transfer Syntax: " << original_xfer.getXferName() << std::endl;
    
    // Obter informações de bits e pixel representation
    Uint16 bits_allocated = 0, bits_stored = 0, samples_per_pixel = 0, pixel_rep = 0;
    dataset->findAndGetUint16(DCM_BitsAllocated, bits_allocated);
    dataset->findAndGetUint16(DCM_BitsStored, bits_stored);
    dataset->findAndGetUint16(DCM_SamplesPerPixel, samples_per_pixel);
    dataset->findAndGetUint16(DCM_PixelRepresentation, pixel_rep);
    
    bool is_signed = (pixel_rep == 1);
    
    // Obter Rescale Slope e Intercept (importante para CT)
    Float64 rescale_slope = 1.0, rescale_intercept = 0.0;
    dataset->findAndGetFloat64(DCM_RescaleSlope, rescale_slope);
    dataset->findAndGetFloat64(DCM_RescaleIntercept, rescale_intercept);
    
    std::cout << "[DEBUG] Bits allocated: " << bits_allocated 
              << ", Bits stored: " << bits_stored 
              << ", Signed: " << (is_signed ? "yes" : "no") << std::endl;
    std::cout << "[DEBUG] Rescale Slope: " << rescale_slope 
              << ", Intercept: " << rescale_intercept << std::endl;
    
    // Criar DicomImage com flags apropriadas
    unsigned long flags = CIF_MayDetachPixelData;
    
    ::DicomImage dcmtk_image(
        static_cast<DcmObject *>(file_format.getDataset()), 
        EXS_Unknown,
        flags);

    if (dcmtk_image.getStatus() != EIS_Normal) {
      return ErrorInfo{DicomError::InvalidImageDimensions,
                       "Failed to load DICOM image",
                       ::DicomImage::getString(dcmtk_image.getStatus())};
    }

    if (!dcmtk_image.isMonochrome()) {
      return ErrorInfo{DicomError::InvalidFormat, "Not a monochrome image", ""};
    }

    ImageData img_data;
    img_data.width = dcmtk_image.getWidth();
    img_data.height = dcmtk_image.getHeight();

    if (img_data.width == 0 || img_data.height == 0) {
      return ErrorInfo{DicomError::InvalidImageDimensions,
                       "Invalid image dimensions", ""};
    }

    img_data.bits_allocated = bits_allocated;
    img_data.bits_stored = bits_stored;
    img_data.samples_per_pixel = samples_per_pixel;
    img_data.is_signed = is_signed;

    size_t pixel_count = img_data.width * img_data.height;
    
    // Obter os dados de pixel usando a representação interna do DCMTK
    // O DCMTK já aplica rescale slope/intercept internamente
    const DiPixel* pixel_data = dcmtk_image.getInterData();
    if (!pixel_data) {
      return ErrorInfo{DicomError::MissingPixelData, "No pixel data found", ""};
    }
    
    EP_Representation rep = pixel_data->getRepresentation();
    std::cout << "[DEBUG] Internal representation: " << static_cast<int>(rep) << std::endl;
    
    // Obter min/max dos dados internos
    double min_val = 0, max_val = 0;
    dcmtk_image.getMinMaxValues(min_val, max_val);
    std::cout << "[DEBUG] DCMTK min/max values: " << min_val << " - " << max_val << std::endl;
    
    // Alocar buffer para pixels normalizados (sempre usar int32 internamente para suportar signed)
    img_data.pixels.resize(pixel_count);
    
    // Calcular o range efetivo dos dados
    double data_range = max_val - min_val;
    if (data_range < 1) data_range = 1;
    
    // Para armazenamento, normalizar para 0-65535 (uint16)
    // Mas manter track do offset para cálculos de window/level
    double scale = 65535.0 / data_range;
    
    // Copiar dados baseado na representação
    const void* raw_data = pixel_data->getData();
    
    if (rep == EPR_Sint16 || rep == EPR_Uint16) {
      const uint16_t* src = static_cast<const uint16_t*>(raw_data);
      if (is_signed) {
        const int16_t* signed_src = static_cast<const int16_t*>(raw_data);
        for (size_t i = 0; i < pixel_count; ++i) {
          double normalized = (static_cast<double>(signed_src[i]) - min_val) * scale;
          img_data.pixels[i] = static_cast<uint16_t>(std::clamp(normalized, 0.0, 65535.0));
        }
      } else {
        for (size_t i = 0; i < pixel_count; ++i) {
          double normalized = (static_cast<double>(src[i]) - min_val) * scale;
          img_data.pixels[i] = static_cast<uint16_t>(std::clamp(normalized, 0.0, 65535.0));
        }
      }
    } else if (rep == EPR_Sint32 || rep == EPR_Uint32) {
      const int32_t* src = static_cast<const int32_t*>(raw_data);
      for (size_t i = 0; i < pixel_count; ++i) {
        double normalized = (static_cast<double>(src[i]) - min_val) * scale;
        img_data.pixels[i] = static_cast<uint16_t>(std::clamp(normalized, 0.0, 65535.0));
      }
    } else {
      // Fallback: usar getOutputData
      dcmtk_image.setMinMaxWindow();
      const void* output_8 = dcmtk_image.getOutputData(8, 0, 0);
      if (output_8) {
        const uint8_t* src = static_cast<const uint8_t*>(output_8);
        for (size_t i = 0; i < pixel_count; ++i) {
          img_data.pixels[i] = static_cast<uint16_t>(src[i]) * 257; // Scale 8-bit to 16-bit
        }
      }
    }
    
    // Inverter se MONOCHROME1
    if (is_monochrome1) {
      std::cout << "[DEBUG] Inverting MONOCHROME1 pixels" << std::endl;
      for (auto& pixel : img_data.pixels) {
        pixel = 65535 - pixel;
      }
    }
    
    uint16_t min_pix = *std::min_element(img_data.pixels.begin(), img_data.pixels.end());
    uint16_t max_pix = *std::max_element(img_data.pixels.begin(), img_data.pixels.end());
    std::cout << "[DEBUG] Final pixel range: " << min_pix << " - " << max_pix << std::endl;

    img_data.is_preprocessed = false;
    img_data.photometric = PhotometricInterpretation::Monochrome2;

    // Extrair window/level do arquivo DICOM
    Float64 file_wc = 0, file_ww = 0;
    bool has_window = false;
    
    if (dataset->findAndGetFloat64(DCM_WindowCenter, file_wc).good() &&
        dataset->findAndGetFloat64(DCM_WindowWidth, file_ww).good() && 
        file_ww > 0) {
      has_window = true;
      std::cout << "[DEBUG] Window from DICOM tags (original): Center=" << file_wc 
                << ", Width=" << file_ww << std::endl;
      
      // Converter window/level do espaço original para o espaço normalizado (0-65535)
      // WC e WW estão no espaço dos valores após rescale (Hounsfield para CT)
      double normalized_wc = (file_wc - min_val) * scale;
      double normalized_ww = file_ww * scale;
      
      img_data.window_center = static_cast<int32_t>(normalized_wc);
      img_data.window_width = static_cast<int32_t>(normalized_ww);
      
      if (is_monochrome1) {
        img_data.window_center = 65535 - img_data.window_center;
      }
      
      std::cout << "[DEBUG] Window (normalized): Center=" << img_data.window_center 
                << ", Width=" << img_data.window_width << std::endl;
    }
    
    // Se não há window/level no arquivo, tentar obter do DCMTK ou calcular
    if (!has_window || img_data.window_width <= 0) {
      double wc = 0, ww = 0;
      
      // Tentar VOI LUT do DCMTK
      if (dcmtk_image.getWindow(wc, ww) && ww > 0) {
        // Converter para espaço normalizado
        double normalized_wc = (wc - min_val) * scale;
        double normalized_ww = ww * scale;
        
        img_data.window_center = static_cast<int32_t>(normalized_wc);
        img_data.window_width = static_cast<int32_t>(normalized_ww);
        
        if (is_monochrome1) {
          img_data.window_center = 65535 - img_data.window_center;
        }
        
        std::cout << "[DEBUG] Window from DCMTK VOI: Center=" << img_data.window_center 
                  << ", Width=" << img_data.window_width << std::endl;
      } else {
        // Calcular automaticamente baseado no histograma
        DicomImageData temp_img;
        temp_img.set_data(img_data);
        temp_img.auto_window_level();
        img_data = temp_img.data();
        std::cout << "[DEBUG] Auto-calculated window: Center=" << img_data.window_center 
                  << ", Width=" << img_data.window_width << std::endl;
      }
    }
    
    // Garantir valores mínimos razoáveis
    if (img_data.window_width < 1) {
      img_data.window_width = 1;
    }

    DicomImageData result;
    result.set_data(std::move(img_data));
    return result;
  }

  Result<DicomImageData, ErrorInfo>
  load_rgb_image(DcmDataset * /*dataset*/,
                 DcmFileFormat &file_format) noexcept {
    ::DicomImage dcmtk_image(static_cast<DcmObject *>(file_format.getDataset()),
                             EXS_Unknown, CIF_MayDetachPixelData);

    if (dcmtk_image.getStatus() != EIS_Normal) {
      return ErrorInfo{DicomError::InvalidImageDimensions,
                       "Failed to load RGB DICOM image",
                       ::DicomImage::getString(dcmtk_image.getStatus())};
    }

    ImageData img_data;
    img_data.width = dcmtk_image.getWidth();
    img_data.height = dcmtk_image.getHeight();
    img_data.photometric = PhotometricInterpretation::RGB;
    img_data.samples_per_pixel = 3;

    const void *rgb_data = dcmtk_image.getOutputData(8);
    if (!rgb_data) {
      return ErrorInfo{DicomError::MissingPixelData,
                       "Failed to get RGB pixel data", ""};
    }

    size_t rgb_size = img_data.width * img_data.height * 3;
    img_data.rgb_pixels.resize(rgb_size);

    std::memcpy(img_data.rgb_pixels.data(), rgb_data, rgb_size);

    img_data.window_center = 128;
    img_data.window_width = 256;

    DicomImageData result;
    result.set_data(std::move(img_data));
    return result;
  }

  void extract_window_level(DcmDataset *dataset, ImageData &img_data) noexcept {
    Float64 window_center = 0, window_width = 0;

    if (dataset->findAndGetFloat64(DCM_WindowCenter, window_center).good()) {
      img_data.window_center = static_cast<int32_t>(window_center);
    }

    if (dataset->findAndGetFloat64(DCM_WindowWidth, window_width).good()) {
      img_data.window_width = static_cast<int32_t>(window_width);
    }
  }

  DicomMetadata extract_metadata(DcmDataset *dataset) noexcept {
    DicomMetadata meta;
    OFString str_value;
    Uint16 uint16_value;
    Uint32 uint32_value;
    Float64 float_value;
    Sint32 sint32_value;

    if (dataset->findAndGetOFString(DCM_PatientName, str_value).good()) {
      meta.patient_name = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_PatientID, str_value).good()) {
      meta.patient_id = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_PatientBirthDate, str_value).good()) {
      meta.patient_birth_date = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_PatientSex, str_value).good()) {
      meta.patient_sex = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_PatientAge, str_value).good()) {
      meta.patient_age = str_value.c_str();
    }

    if (dataset->findAndGetOFString(DCM_StudyDate, str_value).good()) {
      meta.study_date = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_StudyTime, str_value).good()) {
      meta.study_time = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_StudyDescription, str_value).good()) {
      meta.study_description = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_StudyInstanceUID, str_value).good()) {
      meta.study_instance_uid = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_AccessionNumber, str_value).good()) {
      meta.accession_number = str_value.c_str();
    }

    if (dataset->findAndGetOFString(DCM_SeriesDate, str_value).good()) {
      meta.series_date = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_SeriesTime, str_value).good()) {
      meta.series_time = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_SeriesDescription, str_value).good()) {
      meta.series_description = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_SeriesInstanceUID, str_value).good()) {
      meta.series_instance_uid = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_SeriesNumber, str_value).good()) {
      meta.series_number = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_Modality, str_value).good()) {
      meta.modality = str_value.c_str();
    }

    if (dataset->findAndGetOFString(DCM_Manufacturer, str_value).good()) {
      meta.manufacturer = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_ManufacturerModelName, str_value)
            .good()) {
      meta.manufacturer_model_name = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_StationName, str_value).good()) {
      meta.station_name = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_InstitutionName, str_value).good()) {
      meta.institution_name = str_value.c_str();
    }

    if (dataset->findAndGetUint32(DCM_Rows, uint32_value).good()) {
      meta.rows = uint32_value;
    }
    if (dataset->findAndGetUint32(DCM_Columns, uint32_value).good()) {
      meta.columns = uint32_value;
    }
    if (dataset->findAndGetUint16(DCM_BitsAllocated, uint16_value).good()) {
      meta.bits_allocated = uint16_value;
    }
    if (dataset->findAndGetUint16(DCM_BitsStored, uint16_value).good()) {
      meta.bits_stored = uint16_value;
    }
    if (dataset->findAndGetUint16(DCM_HighBit, uint16_value).good()) {
      meta.high_bit = uint16_value;
    }
    if (dataset->findAndGetUint16(DCM_SamplesPerPixel, uint16_value).good()) {
      meta.samples_per_pixel = uint16_value;
    }
    if (dataset->findAndGetOFString(DCM_PhotometricInterpretation, str_value)
            .good()) {
      meta.photometric_interpretation = str_value.c_str();
    }
    if (dataset->findAndGetOFString(DCM_PixelSpacing, str_value).good()) {
      meta.pixel_spacing = str_value.c_str();
    }
    if (dataset->findAndGetFloat64(DCM_SliceThickness, float_value).good()) {
      meta.slice_thickness = float_value;
    }

    if (dataset->findAndGetSint32(DCM_WindowCenter, sint32_value).good()) {
      meta.window_center = sint32_value;
    }
    if (dataset->findAndGetSint32(DCM_WindowWidth, sint32_value).good()) {
      meta.window_width = sint32_value;
    }
    if (dataset->findAndGetOFString(DCM_WindowCenterWidthExplanation, str_value)
            .good()) {
      meta.window_explanation = str_value.c_str();
    }

    return meta;
  }
};

DcmtkReader::DcmtkReader() noexcept : impl_(std::make_unique<Impl>()) {}

DcmtkReader::~DcmtkReader() noexcept = default;

DcmtkReader::DcmtkReader(DcmtkReader &&) noexcept = default;
DcmtkReader &DcmtkReader::operator=(DcmtkReader &&) noexcept = default;

Result<DicomImageData, ErrorInfo>
DcmtkReader::load_image(const std::filesystem::path &path) {
  return impl_->load_image_impl(path);
}

Result<DicomMetadata, ErrorInfo>
DcmtkReader::load_metadata(const std::filesystem::path &path) {
  return impl_->load_metadata_impl(path);
}

Result<std::pair<DicomImageData, DicomMetadata>, ErrorInfo>
DcmtkReader::load_complete(const std::filesystem::path &path) {
  auto img_result = load_image(path);
  if (img_result.is_error()) {
    return img_result.error();
  }

  auto meta_result = load_metadata(path);
  if (meta_result.is_error()) {
    return meta_result.error();
  }

  return std::make_pair(std::move(img_result.value()),
                        std::move(meta_result.value()));
}