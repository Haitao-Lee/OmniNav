#include "GeometryUtils.h"
#include <random>
#include <algorithm>
#include <numeric>
#include <vtkLandmarkTransform.h>
#include <vtkPoints.h>
#include <vtkSmartPointer.h>
#include <Eigen/Geometry> 
#include <QDebug>

// --- Mesh Volume Calculation ---
double GeometryUtils::volumeOfMesh(vtkSmartPointer<vtkPolyData> polydata) {
    if (!polydata || polydata->GetNumberOfCells() == 0) return 0.0;

    auto massProperties = vtkSmartPointer<vtkMassProperties>::New();
    massProperties->SetInputData(polydata);
    massProperties->Update();

    return massProperties->GetVolume();
}

// --- SVD Plane Fitting (Internal) ---
std::pair<Eigen::Vector3d, Eigen::Vector3d> GeometryUtils::SVDPlaneFit(const Eigen::MatrixXd& points) {
    Eigen::Vector3d center = points.colwise().mean();
    Eigen::MatrixXd centered = points.rowwise() - center.transpose();
    
    // Singular Value Decomposition
    Eigen::JacobiSVD<Eigen::MatrixXd> svd(centered, Eigen::ComputeThinV);
    // The normal is the eigenvector corresponding to the smallest eigenvalue
    Eigen::Vector3d normal = svd.matrixV().col(2); 

    return {center, normal.normalized()};
}

// --- Least Squares Plane Fitting ---
PlaneParams GeometryUtils::fitPlaneByNorm2(const Eigen::MatrixXd& points) {
    Eigen::MatrixXd A(points.rows(), 3);
    Eigen::VectorXd B(points.rows());

    for (int i = 0; i < points.rows(); ++i) {
        A(i, 0) = points(i, 0);
        A(i, 1) = points(i, 1);
        A(i, 2) = 1.0;
        B(i) = points(i, 2);
    }

    // Solve using SVD for stability
    Eigen::Vector3d p = A.jacobiSvd(Eigen::ComputeThinU | Eigen::ComputeThinV).solve(B);
    double a = p[0], b = p[1], c = p[2];

    Eigen::Vector3d mean = points.colwise().mean();
    Eigen::Vector3d minPt = points.colwise().minCoeff();
    Eigen::Vector3d maxPt = points.colwise().maxCoeff();

    PlaneParams out;
    out.center = Eigen::Vector3d(mean[0], mean[1], a * mean[0] + b * mean[1] + c);
    out.origin = Eigen::Vector3d(minPt[0], minPt[1], a * minPt[0] + b * minPt[1] + c);
    out.normal = Eigen::Vector3d(a, b, -1.0).normalized();
    out.pt1 = Eigen::Vector3d(maxPt[0], minPt[1], a * maxPt[0] + b * minPt[1] + c);
    out.pt2 = Eigen::Vector3d(minPt[0], maxPt[1], a * minPt[0] + b * maxPt[1] + c);

    return out;
}

// --- RANSAC Plane Fitting ---
PlaneParams GeometryUtils::ransacPlaneFit(const Eigen::MatrixXd& points, int ransacN, double maxDst, int maxTrials) {
    int num = points.rows();
    if (num < ransacN) return PlaneParams();

    double bestInliersRatio = -1.0;
    Eigen::Vector3d bestCenter, bestNormal;

    std::vector<int> indices(num);
    std::iota(indices.begin(), indices.end(), 0);
    std::random_device rd;
    std::mt19937 g(rd());

    for (int i = 0; i < maxTrials; ++i) {
        std::shuffle(indices.begin(), indices.end(), g);
        Eigen::MatrixXd sample(ransacN, 3);
        for (int j = 0; j < ransacN; ++j) sample.row(j) = points.row(indices[j]);

        auto [c, n] = SVDPlaneFit(sample);
        
        // Count inliers
        int inliersCount = 0;
        for (int j = 0; j < num; ++j) {
            double d = std::abs((points.row(j).transpose() - c).dot(n));
            if (d < maxDst) inliersCount++;
        }

        double ratio = (double)inliersCount / num;
        if (ratio > bestInliersRatio) {
            bestInliersRatio = ratio;
            bestCenter = c;
            bestNormal = n;
        }
    }

    PlaneParams out;
    out.center = bestCenter;
    out.normal = bestNormal;
    return out;
}

// --- 3D Segment Distance ---
double GeometryUtils::segment3DDist(const Eigen::Vector3d& p1, const Eigen::Vector3d& p2, 
                                   const Eigen::Vector3d& q1, const Eigen::Vector3d& q2) {
    Eigen::Vector3d u = p2 - p1;
    Eigen::Vector3d v = q2 - q1;
    Eigen::Vector3d w = p1 - q1;

    double a = u.dot(u);
    double b = u.dot(v);
    double c = v.dot(v);
    double d = u.dot(w);
    double e = v.dot(w);
    double D = a * c - b * b;
    double sc, sN, sD = D;
    double tc, tN, tD = D;

    if (D < 1e-8) { // Parallel
        sN = 0.0; sD = 1.0; tN = e; tD = c;
    } else {
        sN = (b * e - c * d);
        tN = (a * e - b * d);
        if (sN < 0.0) { sN = 0.0; tN = e; tD = c; }
        else if (sN > sD) { sN = sD; tN = e + b; tD = c; }
    }

    if (tN < 0.0) {
        tN = 0.0;
        if (-d < 0.0) sN = 0.0;
        else if (-d > a) sN = sD;
        else { sN = -d; sD = a; }
    } else if (tN > tD) {
        tN = tD;
        if ((-d + b) < 0.0) sN = 0;
        else if ((-d + b) > a) sN = sD;
        else { sN = (-d + b); sD = a; }
    }

    sc = (std::abs(sN) < 1e-8 ? 0.0 : sN / sD);
    tc = (std::abs(tN) < 1e-8 ? 0.0 : tN / tD);

    Eigen::Vector3d dP = w + (sc * u) - (tc * v);
    return dP.norm();
}

vtkSmartPointer<vtkPolyData> GeometryUtils::eigenToVtkPoly(const Eigen::MatrixXd& points) {
    vtkSmartPointer<vtkPoints> vtk_pts = vtkSmartPointer<vtkPoints>::New();
    for (int i = 0; i < points.rows(); ++i) {
        vtk_pts->InsertNextPoint(points(i, 0), points(i, 1), points(i, 2));
    }
    vtkSmartPointer<vtkPolyData> poly = vtkSmartPointer<vtkPolyData>::New();
    poly->SetPoints(vtk_pts);
    return poly;
}


Eigen::Matrix4d GeometryUtils::computeRegistration(const Eigen::MatrixXd& sourcePts, const Eigen::MatrixXd& targetPts) 
{
    int n = sourcePts.rows();
    if (n == 0 || sourcePts.rows() != targetPts.rows()) return Eigen::Matrix4d::Identity();

    Eigen::Matrix4d resultMat = Eigen::Matrix4d::Identity();

    // === 情况 A: 只有 1 个点 (平移对齐) ===
    if (n == 1) {
        // 直觉解：只算位移 T = Target - Source
        Eigen::Vector3d s = sourcePts.row(0);
        Eigen::Vector3d t = targetPts.row(0);
        Eigen::Vector3d translation = t - s;
        resultMat.block<3, 1>(0, 3) = translation;
    }
    // === 情况 B: 只有 2 个点 (锚点对齐 + 轴对齐) ===
    // 以第一个点为锚点 (Anchor)，针尖不动，甩动针尾 
    else if (n == 2) {
        // 1. 获取点
        Eigen::Vector3d s1 = sourcePts.row(0); // 锚点 (针尖)
        Eigen::Vector3d s2 = sourcePts.row(1); // 方向点 (针尾)
        Eigen::Vector3d t1 = targetPts.row(0);
        Eigen::Vector3d t2 = targetPts.row(1);

        // 2. 计算向量
        Eigen::Vector3d v_source = s2 - s1;
        Eigen::Vector3d v_target = t2 - t1;

        // 3. 计算旋转 (将 Source向量 旋转到 Target向量)
        v_source.normalize();
        v_target.normalize();
        
        // FromTwoVectors 计算最小旋转，把 v_source 转到 v_target
        Eigen::Quaterniond q = Eigen::Quaterniond::FromTwoVectors(v_source, v_target);
        Eigen::Matrix3d R = q.toRotationMatrix();

        // 4. 计算平移
        // 逻辑：先旋转，旋转后 S1 可能会跑偏，我们需要把旋转后的 S1 移回 T1
        // T = T1 - (R * S1)
        Eigen::Vector3d translation = t1 - (R * s1);

        resultMat.block<3, 3>(0, 0) = R;
        resultMat.block<3, 1>(0, 3) = translation;
    }
    // === 情况 C: 3 个及以上点 (SVD 刚体配准) ===
    else {
        // ... (保持原本的 VTK/SVD 逻辑，这是数学最优解) ...
        vtkSmartPointer<vtkPoints> vtkSourcePts = vtkSmartPointer<vtkPoints>::New();
        vtkSmartPointer<vtkPoints> vtkTargetPts = vtkSmartPointer<vtkPoints>::New();
        for (int i = 0; i < n; ++i) {
            vtkSourcePts->InsertNextPoint(sourcePts(i, 0), sourcePts(i, 1), sourcePts(i, 2));
            vtkTargetPts->InsertNextPoint(targetPts(i, 0), targetPts(i, 1), targetPts(i, 2));
        }
        vtkSmartPointer<vtkLandmarkTransform> landmarkTransform = vtkSmartPointer<vtkLandmarkTransform>::New();
        landmarkTransform->SetSourceLandmarks(vtkSourcePts);
        landmarkTransform->SetTargetLandmarks(vtkTargetPts);
        landmarkTransform->SetModeToRigidBody();
        landmarkTransform->Update();
        
        vtkSmartPointer<vtkMatrix4x4> vtkMat = landmarkTransform->GetMatrix();
        for (int i = 0; i < 4; ++i) 
            for (int j = 0; j < 4; ++j) 
                resultMat(i, j) = vtkMat->GetElement(i, j);
    }

    return resultMat;
}