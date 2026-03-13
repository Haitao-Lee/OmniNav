#ifndef SOPHIAR_URMATH_H
#define SOPHIAR_URMATH_H

#include "RobotTypes.h"

#include <Eigen/Geometry>

namespace URMath {
    Eigen::Isometry3d Pose2Transform(CRRobotVector6T pose);

    void Transform2Pose(const Eigen::Isometry3d &transform, RobotVector6 &pose);

    PRobotVector6T Transform2Pose(const Eigen::Isometry3d &transform);
}

#endif //SOPHIAR_URMATH_H
