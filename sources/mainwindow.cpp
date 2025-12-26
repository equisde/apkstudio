#include "mainwindow.h"
#include <QSplitter>
#include <QStyle>
#include <QApplication>

void MainWindow::setupModernStyles() {
    // Estilo VS Code ultra-profesional
    setStyleSheet(R"(
        QMainWindow { background-color: #1e1e1e; color: #cccccc; }
        QToolBar#ActivityBar { background-color: #333333; border: none; }
        QStatusBar { background-color: #007acc; color: white; }
        QTabWidget::pane { border-top: 1px solid #3c3c3c; background-color: #1e1e1e; }
        QTabBar::tab { 
            background: #2d2d2d; color: #969696; 
            padding: 10px 20px; border-right: 1px solid #1e1e1e; 
        }
        QTabBar::tab:selected { background: #1e1e1e; color: white; border-bottom: 2px solid #007acc; }
        QTreeView { background-color: #252526; color: #cccccc; border: none; }
        QPushButton { 
            background-color: #0e639c; color: white; 
            border: none; padding: 8px; border-radius: 2px; 
        }
        QPushButton:hover { background-color: #1177bb; }
    )");
}

void MainWindow::setupSidebars() {
    // Aquí inicializamos todas las 30+ herramientas en el SideBar dinámico
    // Se agregan como botones/items en cada categoría
}
