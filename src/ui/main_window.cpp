#include "main_window.hpp"
#include <QMenuBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QSplitter>
#include <QScrollArea>
#include <QImage>
#include <QPixmap>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , dicom_reader_(std::make_unique<DcmtkReader>())
    , image_loaded_(false)
    , current_window_center_(0)
    , current_window_width_(0)
{
    setWindowTitle("DICOM Viewer");
    resize(1200, 800);
    
    setup_ui();
    setup_menu();
    create_toolbar();
    
    status_bar_ = statusBar();
    status_bar_->showMessage("Ready");
}

MainWindow::~MainWindow() = default;

void MainWindow::setup_ui() {
    // Central widget with splitter
    auto* central = new QWidget(this);
    auto* main_layout = new QHBoxLayout(central);
    
    auto* splitter = new QSplitter(Qt::Horizontal, this);
    
    // Left side - Image display
    auto* image_container = new QWidget();
    auto* image_layout = new QVBoxLayout(image_container);
    
    // Scroll area for image
    auto* scroll_area = new QScrollArea();
    scroll_area->setWidgetResizable(true);
    scroll_area->setAlignment(Qt::AlignCenter);
    
    image_label_ = new QLabel();
    image_label_->setAlignment(Qt::AlignCenter);
    image_label_->setMinimumSize(400, 400);
    image_label_->setText("No image loaded\n\nFile > Open to load a DICOM file");
    image_label_->setStyleSheet("QLabel { background-color: #2b2b2b; color: #888; font-size: 14px; }");
    
    scroll_area->setWidget(image_label_);
    image_layout->addWidget(scroll_area);
    
    // Window/Level controls
    auto* controls_group = new QGroupBox("Window/Level");
    auto* controls_layout = new QVBoxLayout();
    
    // Window Center
    auto* wc_layout = new QHBoxLayout();
    wc_layout->addWidget(new QLabel("Window Center:"));
    
    window_center_slider_ = new QSlider(Qt::Horizontal);
    window_center_slider_->setRange(-2048, 4096);
    window_center_slider_->setValue(0);
    window_center_slider_->setEnabled(false);
    wc_layout->addWidget(window_center_slider_);
    
    window_center_spin_ = new QSpinBox();
    window_center_spin_->setRange(-2048, 4096);
    window_center_spin_->setValue(0);
    window_center_spin_->setEnabled(false);
    wc_layout->addWidget(window_center_spin_);
    
    controls_layout->addLayout(wc_layout);
    
    // Window Width
    auto* ww_layout = new QHBoxLayout();
    ww_layout->addWidget(new QLabel("Window Width:"));
    
    window_width_slider_ = new QSlider(Qt::Horizontal);
    window_width_slider_->setRange(1, 4096);
    window_width_slider_->setValue(256);
    window_width_slider_->setEnabled(false);
    ww_layout->addWidget(window_width_slider_);
    
    window_width_spin_ = new QSpinBox();
    window_width_spin_->setRange(1, 4096);
    window_width_spin_->setValue(256);
    window_width_spin_->setEnabled(false);
    ww_layout->addWidget(window_width_spin_);
    
    controls_layout->addLayout(ww_layout);
    
    // Buttons
    auto* btn_layout = new QHBoxLayout();
    
    reset_window_btn_ = new QPushButton("Reset");
    reset_window_btn_->setEnabled(false);
    btn_layout->addWidget(reset_window_btn_);
    
    auto_window_btn_ = new QPushButton("Auto");
    auto_window_btn_->setEnabled(false);
    btn_layout->addWidget(auto_window_btn_);
    
    controls_layout->addLayout(btn_layout);
    controls_group->setLayout(controls_layout);
    
    image_layout->addWidget(controls_group);
    
    // Right side - Metadata
    metadata_text_ = new QTextEdit();
    metadata_text_->setReadOnly(true);
    metadata_text_->setMinimumWidth(300);
    metadata_text_->setMaximumWidth(400);
    metadata_text_->setFont(QFont("Courier", 9));
    
    splitter->addWidget(image_container);
    splitter->addWidget(metadata_text_);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 1);
    
    main_layout->addWidget(splitter);
    setCentralWidget(central);
    
    // Connect signals
    connect(window_center_slider_, &QSlider::valueChanged,
            this, &MainWindow::on_window_center_changed);
    connect(window_center_spin_, QOverload<int>::of(&QSpinBox::valueChanged),
            window_center_slider_, &QSlider::setValue);
    
    connect(window_width_slider_, &QSlider::valueChanged,
            this, &MainWindow::on_window_width_changed);
    connect(window_width_spin_, QOverload<int>::of(&QSpinBox::valueChanged),
            window_width_slider_, &QSlider::setValue);
    
    connect(reset_window_btn_, &QPushButton::clicked,
            this, &MainWindow::on_reset_window);
    connect(auto_window_btn_, &QPushButton::clicked,
            this, &MainWindow::on_auto_window);
}

void MainWindow::setup_menu() {
    auto* file_menu = menuBar()->addMenu("&File");
    
    auto* open_action = file_menu->addAction("&Open DICOM...");
    open_action->setShortcut(QKeySequence::Open);
    connect(open_action, &QAction::triggered, this, &MainWindow::on_open_file);
    
    file_menu->addSeparator();
    
    auto* exit_action = file_menu->addAction("E&xit");
    exit_action->setShortcut(QKeySequence::Quit);
    connect(exit_action, &QAction::triggered, this, &QWidget::close);
    
    auto* view_menu = menuBar()->addMenu("&View");
    
    auto* metadata_action = view_menu->addAction("Toggle &Metadata Panel");
    metadata_action->setShortcut(Qt::Key_M);
    connect(metadata_action, &QAction::triggered, this, &MainWindow::toggle_metadata_panel);
}

void MainWindow::create_toolbar() {
    auto* toolbar = addToolBar("Main Toolbar");
    
    auto* open_action = toolbar->addAction("Open");
    connect(open_action, &QAction::triggered, this, &MainWindow::on_open_file);
    
    toolbar->addSeparator();
    
    auto* reset_action = toolbar->addAction("Reset W/L");
    connect(reset_action, &QAction::triggered, this, &MainWindow::on_reset_window);
    
    auto* auto_action = toolbar->addAction("Auto W/L");
    connect(auto_action, &QAction::triggered, this, &MainWindow::on_auto_window);
}

void MainWindow::on_open_file() {
    QString filename = QFileDialog::getOpenFileName(
        this,
        "Open DICOM File",
        "",
        "DICOM Files (*.dcm *.DCM *.dicom);;All Files (*)"
    );
    
    if (filename.isEmpty()) {
        return;
    }
    
    status_bar_->showMessage("Loading DICOM file...");
    
    auto result = dicom_reader_->load_complete(filename.toStdString());
    
    if (result.is_error()) {
        display_error(result.error());
        status_bar_->showMessage("Failed to load DICOM file");
        return;
    }
    
    auto [image, metadata] = std::move(result.value());
    
    current_image_ = std::move(image);
    current_metadata_ = std::move(metadata);
    image_loaded_ = true;
    
    // Set initial window/level from image
    current_window_center_ = current_image_.data().window_center;
    current_window_width_ = current_image_.data().window_width;
    
    update_window_controls();
    display_image();
    update_metadata_display();
    
    // Enable controls
    window_center_slider_->setEnabled(true);
    window_center_spin_->setEnabled(true);
    window_width_slider_->setEnabled(true);
    window_width_spin_->setEnabled(true);
    reset_window_btn_->setEnabled(true);
    auto_window_btn_->setEnabled(true);
    
    status_bar_->showMessage(
        QString("Loaded: %1x%2 %3")
            .arg(current_image_.data().width)
            .arg(current_image_.data().height)
            .arg(current_image_.data().is_rgb() ? "RGB" : "Grayscale")
    );
}

void MainWindow::on_window_center_changed(int value) {
    if (!image_loaded_) return;
    
    current_window_center_ = value;
    window_center_spin_->blockSignals(true);
    window_center_spin_->setValue(value);
    window_center_spin_->blockSignals(false);
    
    update_image_display();
}

void MainWindow::on_window_width_changed(int value) {
    if (!image_loaded_) return;
    
    current_window_width_ = value;
    window_width_spin_->blockSignals(true);
    window_width_spin_->setValue(value);
    window_width_spin_->blockSignals(false);
    
    update_image_display();
}

void MainWindow::on_reset_window() {
    if (!image_loaded_) return;
    
    current_window_center_ = current_image_.data().window_center;
    current_window_width_ = current_image_.data().window_width;
    
    update_window_controls();
    update_image_display();
}

void MainWindow::on_auto_window() {
    if (!image_loaded_) return;
    
    current_image_.auto_window_level();
    current_window_center_ = current_image_.data().window_center;
    current_window_width_ = current_image_.data().window_width;
    
    update_window_controls();
    update_image_display();
}

void MainWindow::toggle_metadata_panel() {
    metadata_text_->setVisible(!metadata_text_->isVisible());
}

void MainWindow::display_error(const ErrorInfo& error) {
    QString error_msg = QString("Error: %1").arg(QString::fromStdString(error.message));
    
    if (!error.details.empty()) {
        error_msg += QString("\n\nDetails: %1").arg(QString::fromStdString(error.details));
    }
    
    // Add suggestions based on error type
    switch (error.code) {
        case DicomError::UnsupportedTransferSyntax:
            error_msg += "\n\nThis file may be compressed with an unsupported codec.";
            error_msg += "\nTry converting it to uncompressed format using a DICOM tool.";
            break;
        case DicomError::InvalidFormat:
            error_msg += "\n\nMake sure this is a valid DICOM file (.dcm).";
            break;
        case DicomError::UnsupportedPhotometricInterpretation:
            error_msg += "\n\nThe color format of this image is not supported.";
            error_msg += "\nSupported formats: MONOCHROME1, MONOCHROME2, RGB";
            break;
        default:
            break;
    }
    
    QMessageBox::critical(this, "DICOM Error", error_msg);
}

void MainWindow::display_image() {
    update_image_display();
}

void MainWindow::update_image_display() {
    if (!image_loaded_) return;
    
    const auto& img_data = current_image_.data();
    
    QImage q_image;
    
    if (img_data.is_rgb()) {
        // RGB image
        auto rgb_buffer = current_image_.to_rgb_display_buffer();
        q_image = QImage(
            rgb_buffer.data(),
            img_data.width,
            img_data.height,
            img_data.width * 3,
            QImage::Format_RGB888
        );
        q_image = q_image.copy();  // Deep copy
    } else {
        // Grayscale image with window/level
        auto display_buffer = current_image_.to_display_buffer(
            current_window_center_,
            current_window_width_
        );
        
        q_image = QImage(
            display_buffer.data(),
            img_data.width,
            img_data.height,
            img_data.width,
            QImage::Format_Grayscale8
        );
        q_image = q_image.copy();  // Deep copy
    }
    
    // Scale image to fit in the label while maintaining aspect ratio
    QPixmap pixmap = QPixmap::fromImage(q_image);
    
    // Get available space in scroll area
    QSize available_size = image_label_->parentWidget()->size();
    
    // Scale pixmap to fit, maintaining aspect ratio
    if (pixmap.width() > available_size.width() || pixmap.height() > available_size.height()) {
        pixmap = pixmap.scaled(
            available_size,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
    }
    
    image_label_->setPixmap(pixmap);
    image_label_->resize(pixmap.size());
}

void MainWindow::update_metadata_display() {
    QString metadata_str = QString::fromStdString(current_metadata_.to_string());
    metadata_text_->setText(metadata_str);
}

void MainWindow::update_window_controls() {
    window_center_slider_->blockSignals(true);
    window_center_spin_->blockSignals(true);
    window_width_slider_->blockSignals(true);
    window_width_spin_->blockSignals(true);
    
    window_center_slider_->setValue(current_window_center_);
    window_center_spin_->setValue(current_window_center_);
    window_width_slider_->setValue(current_window_width_);
    window_width_spin_->setValue(current_window_width_);
    
    window_center_slider_->blockSignals(false);
    window_center_spin_->blockSignals(false);
    window_width_slider_->blockSignals(false);
    window_width_spin_->blockSignals(false);
}

#include "main_window.moc"