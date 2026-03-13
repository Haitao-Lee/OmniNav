#ifndef VISUALIZATIONUTILS_H
#define VISUALIZATIONUTILS_H

#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>

// Qt Includes
#include <QWidget>
#include <QLabel>
#include <QSlider>
#include <QString>
#include <QPlainTextEdit>

// VTK Includes
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkAxesActor.h>
#include <vtkMatrix4x4.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkRenderWindowInteractor.h>

class VisualizationUtils {
public:
    // --- VTK Geometry & Actors ---
    static vtkSmartPointer<vtkAxesActor> getAxesActor(double colors[3][3], double length = 1.5);
    
    static vtkSmartPointer<vtkPolyData> getCylinderPolydata(Eigen::Vector3d center, Eigen::Vector3d direct, 
                                                          double radius = 5.0, double length = 10.0, int resolution = 6);
    
    static vtkSmartPointer<vtkActor> getCylinderActor(Eigen::Vector3d center, Eigen::Vector3d direct, 
                                                     double radius = 5.0, double length = 10.0, int resolution = 6, 
                                                     double color[3] = nullptr, double opacity = 1.0);

    static vtkSmartPointer<vtkActor> getDashActor(Eigen::Vector3d center, Eigen::Vector3d direct, double length, int lengthRate = 20);

    static vtkSmartPointer<vtkPolyData> getLinePolydata(Eigen::Vector3d start, Eigen::Vector3d end);
    static vtkSmartPointer<vtkActor> getLineActor(Eigen::Vector3d start, Eigen::Vector3d end, double color[3] = nullptr, double linewidth = 2.0);

    static vtkSmartPointer<vtkPolyData> getSpherePolydata(Eigen::Vector3d center, double radius = 3.0);
    static vtkSmartPointer<vtkActor> getSphereActor(vtkSmartPointer<vtkPolyData> polydata, double color[3] = nullptr, double opacity = 1.0, bool visible = true);

    static vtkSmartPointer<vtkPolyData> getNeedlePolyData(double radius, double length, QString direction = "Z-");

    // --- Anatomical Orientation Marker ---
    static vtkSmartPointer<vtkOrientationMarkerWidget> getOrientationMarker(vtkRenderWindowInteractor* iren);

    // --- Projection Logic ---
    static vtkSmartPointer<vtkActor> getPrjActor(vtkSmartPointer<vtkPolyData> polyData, Eigen::Vector3d cutterCenter, 
                                                Eigen::Vector3d viewCenter, Eigen::Vector3d normal, double offset);

    // --- Qt UI Widgets ---
    static QWidget* getColorWidget(int color[3]);
    static std::pair<QWidget*, QSlider*> getOpacityWidget();
    static QWidget* getVisibleWidget(bool visible);

    static vtkSmartPointer<vtkPolyData> getTubePolyData(
        Eigen::Vector3d start, 
        Eigen::Vector3d end, 
        double radius = 1.0, 
        int resolution = 20
    );

private:
    static vtkSmartPointer<vtkMatrix4x4> getTransformMatrix(Eigen::Vector3d cutterCenter, Eigen::Vector3d viewCenter, Eigen::Vector3d normal, double offset);
};

#endif // VISUALIZATIONUTILS_H