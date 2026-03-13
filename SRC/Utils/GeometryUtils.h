#ifndef GEOMETRY_UTILS_H
#define GEOMETRY_UTILS_H

#pragma once

#include <vector>
#include <string>
#include <Eigen/Dense>

// VTK Includes
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkMassProperties.h>
#include <vtkIterativeClosestPointTransform.h>
#include <vtkLandmarkTransform.h>
#include <vtkTransform.h>
#include <vtkMatrix4x4.h>
#include <vtkPoints.h>

struct PlaneParams {
    Eigen::Vector3d center;
    Eigen::Vector3d normal;
    Eigen::Vector3d origin;
    Eigen::Vector3d pt1;
    Eigen::Vector3d pt2;
};

class GeometryUtils {
public:
    // --- Mesh Operations ---
    static double volumeOfMesh(vtkSmartPointer<vtkPolyData> polydata);

    // --- Plane Fitting Operations ---
    static PlaneParams fitPlaneByNorm2(const Eigen::MatrixXd& points);
    static PlaneParams ransacPlaneFit(const Eigen::MatrixXd& points, 
                                     int ransacN, 
                                     double maxDst, 
                                     int maxTrials = 500);

    // --- Distance Operations ---
    static double segment3DDist(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2, 
                               const Eigen::Vector3d& q1, const Eigen::Vector3d& q2);

    // --- Registration Operations ---
    static Eigen::Matrix4d computeRegistration(const Eigen::MatrixXd& sourcePts, const Eigen::MatrixXd& targetPts);

private:
    static std::pair<Eigen::Vector3d, Eigen::Vector3d> SVDPlaneFit(const Eigen::MatrixXd& points);
    
    // Helper to convert Eigen to VTK
    static vtkSmartPointer<vtkPolyData> eigenToVtkPoly(const Eigen::MatrixXd& points);
};

#endif // GEOMETRY_UTILS_H