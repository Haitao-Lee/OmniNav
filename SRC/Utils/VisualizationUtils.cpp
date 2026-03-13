#include "VisualizationUtils.h"

// VTK Implementation
#include <vtkCylinderSource.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkLineSource.h>
#include <vtkAppendPolyData.h>
#include <vtkSphereSource.h>
#include <vtkPlane.h>
#include <vtkPlaneCutter.h>
#include <vtkCaptionActor2D.h>
#include <vtkTextProperty.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkAnnotatedCubeActor.h>
#include <vtkPropAssembly.h>
#include <vtkNamedColors.h>
#include <vtkConeSource.h>
#include <vtkTubeFilter.h>

// Qt Implementation
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// --- Axes Actor ---
vtkSmartPointer<vtkAxesActor> VisualizationUtils::getAxesActor(double colors[3][3], double length) {
    auto axes = vtkSmartPointer<vtkAxesActor>::New();
    axes->GetXAxisCaptionActor2D()->GetCaptionTextProperty()->SetColor(colors[0]);
    axes->GetYAxisCaptionActor2D()->GetCaptionTextProperty()->SetColor(colors[1]);
    axes->GetZAxisCaptionActor2D()->GetCaptionTextProperty()->SetColor(colors[2]);
    axes->GetXAxisShaftProperty()->SetColor(colors[0]);
    axes->GetXAxisTipProperty()->SetColor(colors[0]);
    axes->GetYAxisShaftProperty()->SetColor(colors[1]);
    axes->GetYAxisTipProperty()->SetColor(colors[1]);
    axes->GetZAxisShaftProperty()->SetColor(colors[2]);
    axes->GetZAxisTipProperty()->SetColor(colors[2]);
    axes->SetTotalLength(length, length, length);
    return axes;
}

// --- Cylinder Generation ---
vtkSmartPointer<vtkPolyData> VisualizationUtils::getCylinderPolydata(Eigen::Vector3d center, Eigen::Vector3d direct, double radius, double length, int resolution) {
    auto cylinder = vtkSmartPointer<vtkCylinderSource>::New();
    cylinder->SetHeight(length);
    cylinder->SetRadius(radius);
    cylinder->SetResolution(resolution);
    cylinder->Update();

    Eigen::Vector3d norm_direct = direct.normalized();
    Eigen::Vector3d y_axis(0, 1, 0);
    Eigen::Vector3d rotate_axis = y_axis.cross(norm_direct);
    double dot = y_axis.dot(norm_direct);
    double angle = std::acos(std::max(-1.0, std::min(1.0, dot))) * 180.0 / M_PI;

    auto tf = vtkSmartPointer<vtkTransform>::New();
    tf->Translate(center.data());
    if (rotate_axis.norm() > 1e-6) {
        tf->RotateWXYZ(angle, rotate_axis.data());
    }

    auto filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filter->SetInputConnection(cylinder->GetOutputPort());
    filter->SetTransform(tf);
    filter->Update();
    return filter->GetOutput();
}

vtkSmartPointer<vtkActor> VisualizationUtils::getCylinderActor(Eigen::Vector3d center, Eigen::Vector3d direct, double radius, double length, int resolution, double color[3], double opacity) {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(getCylinderPolydata(center, direct, radius, length, resolution));
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (color) actor->GetProperty()->SetColor(color);
    actor->GetProperty()->SetOpacity(opacity);
    return actor;
}

// --- Dotted Line Actor ---
vtkSmartPointer<vtkActor> VisualizationUtils::getDashActor(Eigen::Vector3d center, Eigen::Vector3d direct, double length, int lengthRate) {
    auto append = vtkSmartPointer<vtkAppendPolyData>::New();
    Eigen::Vector3d dir = direct.normalized();
    Eigen::Vector3d start = center - 0.5 * lengthRate * length * dir;

    for (int i = 0; i < 2 * lengthRate; ++i) {
        Eigen::Vector3d p1 = start;
        Eigen::Vector3d p2 = start + 0.375 * length * dir;
        auto line = vtkSmartPointer<vtkLineSource>::New();
        line->SetPoint1(p1.data()); line->SetPoint2(p2.data());
        line->Update();
        append->AddInputData(line->GetOutput());
        start += 0.5 * length * dir;
    }
    append->Update();
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputConnection(append->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(1, 0, 0);
    actor->GetProperty()->SetLineWidth(1.0);
    return actor;
}

// --- Line & Sphere ---
vtkSmartPointer<vtkPolyData> VisualizationUtils::getLinePolydata(Eigen::Vector3d start, Eigen::Vector3d end) {
    auto line = vtkSmartPointer<vtkLineSource>::New();
    line->SetPoint1(start.data()); line->SetPoint2(end.data());
    line->Update(); return line->GetOutput();
}

vtkSmartPointer<vtkActor> VisualizationUtils::getLineActor(Eigen::Vector3d start, Eigen::Vector3d end, double color[3], double linewidth) {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(getLinePolydata(start, end));
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (color) actor->GetProperty()->SetColor(color);
    actor->GetProperty()->SetLineWidth(linewidth);
    return actor;
}

vtkSmartPointer<vtkPolyData> VisualizationUtils::getSpherePolydata(Eigen::Vector3d center, double radius) {
    auto sphere = vtkSmartPointer<vtkSphereSource>::New();
    sphere->SetCenter(center.data()); sphere->SetRadius(radius);
    sphere->Update(); return sphere->GetOutput();
}

vtkSmartPointer<vtkActor> VisualizationUtils::getSphereActor(vtkSmartPointer<vtkPolyData> polydata, double color[3], double opacity, bool visible) {
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polydata);
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    if (color) actor->GetProperty()->SetColor(color);
    actor->GetProperty()->SetOpacity(opacity);
    actor->SetVisibility(visible ? 1 : 0);
    return actor;
}


vtkSmartPointer<vtkPolyData> VisualizationUtils::getNeedlePolyData(double radius, double length, QString direction) {
    double coneHeight = radius * 3.0;
    if (coneHeight > length) coneHeight = length;
    double cylinderHeight = length - coneHeight;

    auto append = vtkSmartPointer<vtkAppendPolyData>::New();

    // 1. 构建基础形状 (默认 Z-, 即针尖在原点，针身在Z轴负方向)
    if (cylinderHeight > 1e-6) {
        auto cylinder = vtkSmartPointer<vtkCylinderSource>::New();
        cylinder->SetHeight(cylinderHeight);
        cylinder->SetRadius(radius);
        cylinder->SetResolution(20);
        cylinder->Update();

        auto tfCyl = vtkSmartPointer<vtkTransform>::New();
        // 移动到 -Z 轴
        tfCyl->Translate(0, 0, -coneHeight - cylinderHeight / 2.0);
        tfCyl->RotateX(90);

        auto filterCyl = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        filterCyl->SetInputConnection(cylinder->GetOutputPort());
        filterCyl->SetTransform(tfCyl);
        filterCyl->Update();
        append->AddInputData(filterCyl->GetOutput());
    }

    if (coneHeight > 1e-6) {
        auto cone = vtkSmartPointer<vtkConeSource>::New();
        cone->SetHeight(coneHeight);
        cone->SetRadius(radius);
        cone->SetResolution(20);
        cone->Update();

        auto tfCone = vtkSmartPointer<vtkTransform>::New();
        // 移动到 -Z 轴靠近原点处
        tfCone->Translate(0, 0, -coneHeight / 2.0);
        tfCone->RotateY(-90); // Cone 默认朝 X，转到 Z

        auto filterCone = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        filterCone->SetInputConnection(cone->GetOutputPort());
        filterCone->SetTransform(tfCone);
        filterCone->Update();
        append->AddInputData(filterCone->GetOutput());
    }

    append->Update();

    // 2. 根据选中的方向应用最终变换
    auto tfFinal = vtkSmartPointer<vtkTransform>::New();
    
    // 基础形状是 "Z-" (0,0,-1)
    if (direction == "Z+") {
        tfFinal->RotateX(180); // 翻转到 +Z
    }
    else if (direction == "X-") {
        tfFinal->RotateY(90);  // -Z 转到 -X
    }
    else if (direction == "X+") {
        tfFinal->RotateY(-90); // -Z 转到 +X
    }
    else if (direction == "Y-") {
        tfFinal->RotateX(90);  // -Z 转到 -Y
    }
    else if (direction == "Y+") {
        tfFinal->RotateX(-90); // -Z 转到 +Y
    }
    // else if (direction == "Z-") {不做任何事，保持默认}

    auto filterFinal = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filterFinal->SetInputConnection(append->GetOutputPort());
    filterFinal->SetTransform(tfFinal);
    filterFinal->Update();

    return filterFinal->GetOutput();
}

// --- Anatomical Marker ---
vtkSmartPointer<vtkOrientationMarkerWidget> VisualizationUtils::getOrientationMarker(vtkRenderWindowInteractor* iren) {
    auto colors = vtkSmartPointer<vtkNamedColors>::New();
    auto cube = vtkSmartPointer<vtkAnnotatedCubeActor>::New();
    cube->SetXPlusFaceText("R"); cube->SetXMinusFaceText("L");
    cube->SetYPlusFaceText("A"); cube->SetYMinusFaceText("P");
    cube->SetZPlusFaceText("S"); cube->SetZMinusFaceText("I");
    cube->GetCubeProperty()->SetColor(colors->GetColor3d("Gainsboro").GetData());
    cube->GetXPlusFaceProperty()->SetColor(colors->GetColor3d("Tomato").GetData());
    cube->GetYPlusFaceProperty()->SetColor(colors->GetColor3d("DeepSkyBlue").GetData());
    cube->GetZPlusFaceProperty()->SetColor(colors->GetColor3d("SeaGreen").GetData());

    double axisColors[3][3] = {{0,1,0}, {0,0,1}, {1,0,0}};
    auto axes = getAxesActor(axisColors, 1.5);

    auto assembly = vtkSmartPointer<vtkPropAssembly>::New();
    assembly->AddPart(axes); assembly->AddPart(cube);

    auto widget = vtkSmartPointer<vtkOrientationMarkerWidget>::New();
    widget->SetOrientationMarker(assembly);
    widget->SetInteractor(iren);
    widget->SetViewport(0.0, 0.0, 0.25, 0.25);
    widget->SetEnabled(1);
    widget->InteractiveOff();
    return widget;
}

// --- Projection Matrix ---
vtkSmartPointer<vtkMatrix4x4> VisualizationUtils::getTransformMatrix(Eigen::Vector3d cutterCenter, Eigen::Vector3d viewCenter, Eigen::Vector3d normal, double offset) {
    auto tf = vtkSmartPointer<vtkTransform>::New();
    tf->PostMultiply();
    tf->Translate(-cutterCenter[0], -cutterCenter[1], -cutterCenter[2]);
    if (std::abs(normal[0]) > 0.9) { // X Normal
        tf->RotateY(90); tf->RotateZ(-90); tf->Scale(1, -1, 1);
        tf->Translate(cutterCenter[1] - viewCenter[1], cutterCenter[2] - viewCenter[2], offset);
    } else if (std::abs(normal[1]) > 0.9) { // Y Normal
        tf->RotateX(90); tf->Scale(1, -1, 1);
        tf->Translate(cutterCenter[0] - viewCenter[0], cutterCenter[2] - viewCenter[2], offset);
    } else { // Z Normal
        tf->Scale(-1, 1, 1);
        tf->Translate(viewCenter[0] - cutterCenter[0], cutterCenter[1] - viewCenter[1], offset);
    }
    return tf->GetMatrix();
}

vtkSmartPointer<vtkActor> VisualizationUtils::getPrjActor(vtkSmartPointer<vtkPolyData> polyData, Eigen::Vector3d cutterCenter, Eigen::Vector3d viewCenter, Eigen::Vector3d normal, double offset) {
    auto plane = vtkSmartPointer<vtkPlane>::New();
    plane->SetOrigin(cutterCenter.data()); plane->SetNormal(normal.data());
    auto cutter = vtkSmartPointer<vtkPlaneCutter>::New();
    cutter->SetPlane(plane); cutter->SetInputData(polyData);
    auto tf = vtkSmartPointer<vtkTransform>::New();
    tf->SetMatrix(getTransformMatrix(cutterCenter, viewCenter, normal, offset));
    auto filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
    filter->SetInputConnection(cutter->GetOutputPort()); filter->SetTransform(tf);
    auto mapper = vtkSmartPointer<vtkCompositePolyDataMapper>::New();
    mapper->SetInputConnection(filter->GetOutputPort());
    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper); actor->GetProperty()->SetLineWidth(2);
    return actor;
}

// --- UI Helpers ---
QWidget* VisualizationUtils::getColorWidget(int color[3]) {
    QLabel* label = new QLabel();
    label->setStyleSheet(QString("background-color: rgb(%1,%2,%3);").arg(color[0]).arg(color[1]).arg(color[2]));
    QWidget* w = new QWidget(); QVBoxLayout* l = new QVBoxLayout(w);
    l->addWidget(label); l->setContentsMargins(0,0,0,0);
    return w;
}

std::pair<QWidget*, QSlider*> VisualizationUtils::getOpacityWidget() {
    QSlider* s = new QSlider(Qt::Horizontal); s->setRange(0, 100); s->setValue(100);
    QWidget* w = new QWidget(); QHBoxLayout* l = new QHBoxLayout(w);
    l->addWidget(s); return {w, s};
}

QWidget* VisualizationUtils::getVisibleWidget(bool visible) {
    QLabel* label = new QLabel();
    label->setStyleSheet(visible ? "background-color: green;" : "background-color: gray;");
    QWidget* w = new QWidget(); QVBoxLayout* l = new QVBoxLayout(w);
    l->addWidget(label); l->setContentsMargins(0,0,0,0);
    return w;
}

vtkSmartPointer<vtkPolyData> VisualizationUtils::getTubePolyData(Eigen::Vector3d start, Eigen::Vector3d end, double radius, int resolution)
{
    // 1. 创建线源
    auto lineSource = vtkSmartPointer<vtkLineSource>::New();
    lineSource->SetPoint1(start.data()); // Eigen 直接转 double*
    lineSource->SetPoint2(end.data());
    lineSource->Update();

    // 2. 管道滤波 (加粗)
    auto tubeFilter = vtkSmartPointer<vtkTubeFilter>::New();
    tubeFilter->SetInputConnection(lineSource->GetOutputPort());
    tubeFilter->SetRadius(radius);       // 设置半径
    tubeFilter->SetNumberOfSides(resolution);
    tubeFilter->CappingOff();             // 开启封口 (实心)
    tubeFilter->Update();

    return tubeFilter->GetOutput();
}