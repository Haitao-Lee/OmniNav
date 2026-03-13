#include "URMath.h"

using namespace Eigen;

Isometry3d URMath::Pose2Transform(CRRobotVector6T pose) {
    auto translate = Translation3d(pose[0], pose[1], pose[2]);
    auto rotateAxis = Vector3d(pose[3], pose[4], pose[5]);
    auto rotateAngle = rotateAxis.norm();
    rotateAxis /= rotateAngle;
    auto rotation = AngleAxisd(rotateAngle, rotateAxis);
    return translate * rotation;
}

void URMath::Transform2Pose(const Isometry3d &transform, RobotVector6 &pose) {
    auto transPart = transform.translation();
    for (Index i = 0; i < 3; ++i) {
        pose[i] = transPart(i);
    }

    auto rotPart = AngleAxisd(transform.rotation());
    auto rotAngle = rotPart.angle();
    auto rotAxis = rotPart.axis();
    rotAxis *= rotAngle;
    for (Index i = 0; i < 3; ++i) {
        pose[i + 3] = rotAxis(i);
    }
}

PRobotVector6T URMath::Transform2Pose(const Isometry3d &transform) {
    auto ret = std::make_shared<RobotVector6>();
    Transform2Pose(transform, *ret);
    return std::move(ret);
}