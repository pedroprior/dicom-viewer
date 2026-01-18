#pragma once

#include <QMainWindow>
#include <QLabel>
#include <QSlider>
#include <QSpinBox>
#include <QTextEdit>
#include <QStatusBar>
#include <QPushButton>
#include <memory>

#include "dcmtk_wrapper.hpp"
#include "dicom_image.hpp"
#include "dicom_metadata.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
    
    std::unique_ptr<IDicomReader> dicom_reader_;
    
    // Current loaded data
    DicomImageData current_image_;
    DicomMetadata current_metadata_;
    bool image_loaded_;
    
    // UI Components
    QLabel* image_label_;
    QTextEdit* metadata_text_;
    QSlider* window_center_slider_;
    QSlider* window_width_slider_;
    QSpinBox* window_center_spin_;
    QSpinBox* window_width_spin_;
    QPushButton* reset_window_btn_;
    QPushButton* auto_window_btn_;
    QStatusBar* status_bar_;
    
    // Current window/level values
    int32_t current_window_center_;
    int32_t current_window_width_;
    
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;
    
private slots:
    void on_open_file();
    void on_window_center_changed(int value);
    void on_window_width_changed(int value);
    void on_reset_window();
    void on_auto_window();
    void toggle_metadata_panel();
    
private:
    void setup_ui();
    void setup_menu();
    void create_toolbar();
    void display_error(const ErrorInfo& error);
    void display_image();
    void update_image_display();
    void update_metadata_display();
    void update_window_controls();
};