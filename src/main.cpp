#include "main_window.hpp"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    
    // Set application metadata
    QApplication::setApplicationName("DICOM Exercise");
    QApplication::setApplicationVersion("1.0");
    QApplication::setOrganizationName("Medical Imaging");
    
    // Set dark theme (optional)
    app.setStyle(QStyleFactory::create("Fusion"));
    
    QPalette dark_palette;
    dark_palette.setColor(QPalette::Window, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::WindowText, Qt::white);
    dark_palette.setColor(QPalette::Base, QColor(25, 25, 25));
    dark_palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::ToolTipBase, Qt::white);
    dark_palette.setColor(QPalette::ToolTipText, Qt::white);
    dark_palette.setColor(QPalette::Text, Qt::white);
    dark_palette.setColor(QPalette::Button, QColor(53, 53, 53));
    dark_palette.setColor(QPalette::ButtonText, Qt::white);
    dark_palette.setColor(QPalette::BrightText, Qt::red);
    dark_palette.setColor(QPalette::Link, QColor(42, 130, 218));
    dark_palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
    dark_palette.setColor(QPalette::HighlightedText, Qt::black);
    
    app.setPalette(dark_palette);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}