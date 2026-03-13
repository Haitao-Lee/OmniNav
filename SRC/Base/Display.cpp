#include "Display.h"
#include <QVTKOpenGLNativeWidget.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkNew.h>

Display::Display(QWidget *parent)
    : QMainWindow(parent)
{
    ui.setupUi(this);

    if (ui.view) {
        vtkNew<vtkGenericOpenGLRenderWindow> renderWindow;
        ui.view->SetRenderWindow(renderWindow);

        ui.view->resize(ui.view->size());

        QSizePolicy sizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(1);
        sizePolicy.setHeightForWidth(ui.view->sizePolicy().hasHeightForWidth());
        
        ui.view->setSizePolicy(sizePolicy);
    }

    if (ui.scrollbar) {
        ui.scrollbar->setStyleSheet("QScrollBar { background: transparent; }");
    }
}

Display::~Display()
{
}