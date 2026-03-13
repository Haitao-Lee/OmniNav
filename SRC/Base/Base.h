#pragma once

#include <QtWidgets/QMainWindow>
#include <QVector>
#include <QSplitter>
#include <QJsonObject>
#include <QLabel>
#include <QPair>
#include <QCloseEvent>
#include <QTimer>

#include "ui_Base.h"
#include "Display.h"
#include "VisualizationUtils.h"

#include <vtkSmartPointer.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkCamera.h>

#include <vtkInteractorStyle.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleImage.h>
#include <vtkCellPicker.h>
#include <vtkCallbackCommand.h>

#include <vtkActor.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkOrientationMarkerWidget.h>

#include <vtkLookupTable.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>

#include <QVTKOpenGLNativeWidget.h>

class Base : public QMainWindow
{
    Q_OBJECT
public:
    explicit Base(QWidget *parent = nullptr);
    ~Base() override;

    Base(const Base&) = delete;
    Base& operator=(const Base&) = delete;

    virtual void init();

    enum ViewMode {
        AXIAL = 0,
        VIEW3D = 1,
        CORNAL = 2,
        SAGITA = 3,
        ALLWIN = 4
    };

    QList<int> m_renderViewIds = {0, 1, 2, 3};

    const QJsonObject& getConfig() const { return m_config; }
    Ui::BaseClass& getUi() { return ui; }
    const QVector<QLabel*>& getRegisMarks() const { return m_regisMarks; }
    QSplitter* getOperVisHsplitter() const { return oper_vis_hsplitter; }
    QSplitter* getWholeVsplitter() const { return whole_vsplitter; }
    ViewMode getViewMode() const { return m_viewMode; }
    int getCrossLineSign() const { return m_cross_line_sign; }
    void setNextRenderTargets(const QList<int>& ids) {
        m_renderViewIds = ids;
    }

    const QVector<vtkSmartPointer<vtkRenderWindow>>& getVtkRenderWindows() const { return m_vtkRenderWindows; }
    const QVector<vtkSmartPointer<vtkRenderer>>& getRenderers() const { return m_renderers; }
    const QVector<vtkSmartPointer<vtkRenderer>>& getContourRenderers() const { return m_contourRenderers; }
    const QVector<vtkSmartPointer<vtkInteractorStyle>>& getStyles() const { return m_styles; }
    const QVector<vtkSmartPointer<vtkRenderWindowInteractor>>& getIrens() const { return m_irens; }
    vtkSmartPointer<vtkCellPicker> getPicker() const { return m_picker; }
    const double* getLineCenter() const { return m_lineCenter; }
    const QVector<vtkSmartPointer<vtkActor>>& getLineActors() const { return m_lineActors; }
    vtkSmartPointer<vtkOrientationMarkerWidget> get3DOrientationWidget() const { return m_3DOrientationWidget; }

protected:
    Ui::BaseClass ui;
    QJsonObject m_config;
    QVector<Display*> m_uiDisplays;
    QVector<QLabel*> m_regisMarks;
    QSplitter* oper_vis_hsplitter = nullptr;
    QSplitter* whole_vsplitter = nullptr;
    ViewMode m_viewMode;
    int m_cross_line_sign;

    QVector<vtkSmartPointer<vtkRenderWindow>> m_vtkRenderWindows;
    QVector<vtkSmartPointer<vtkRenderer>> m_renderers;
    QVector<vtkSmartPointer<vtkRenderer>> m_contourRenderers;
    QVector<vtkSmartPointer<vtkInteractorStyle>> m_styles;
    QVector<vtkSmartPointer<vtkRenderWindowInteractor>> m_irens;
    vtkSmartPointer<vtkCellPicker> m_picker;
    double m_lineCenter[3];
    QVector<vtkSmartPointer<vtkActor>> m_lineActors;
    vtkSmartPointer<vtkOrientationMarkerWidget> m_3DOrientationWidget;

    QTimer* m_renderTimer = nullptr;
    bool m_renderPending = false;

    void closeEvent(QCloseEvent* e) override;
    void updateViews();
    void updateView(int view_num);

    void initSplitters();
    void initVTKviews();
    void initVTKProperty();
    void initLights();
    void loadConfig();
    void initRenderTimer();
    void createActions();
    void resetCamera(vtkRenderer* renderer, int parallelProjection = 0, double* viewUp = nullptr);
    void add2DLineActors();
    void reposition2DLineActors();

protected slots:
    void onLayoutChanged(int index);
    void onResetCammera0();
    void onResetCammera1();
    void onResetCammera2();
    void onResetCammera3();
    void onRotateView0();
    void onRotateView1();
    void onRotateView2();
    void onRotateView3();
    void onZoomView0();
    void onZoomView1();
    void onZoomView2();
    void onZoomView3();
    void onCrossLinesUpdate();
    void doThrottledRender();
    void setInteractiveRendering(bool enable);
};