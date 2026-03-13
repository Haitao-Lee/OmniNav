#ifndef SOPHIAR_ROBOTTYPES_H
#define SOPHIAR_ROBOTTYPES_H

#include "EasyDataIO.h"

#include <array>
#include <memory>

template<size_t L>
using RobotVector = std::array<double, L>;

template<size_t L>
using PRobotVectorT = std::shared_ptr<RobotVector<L>>;

template<size_t L>
using CRRobotVectorT = const RobotVector<L> &;

template<size_t L>
bool ReadRobotVector(EasyDataIO &dio, RobotVector<L> &vector);

template<size_t L>
bool WriteRobotVector(EasyDataIO &dio, const RobotVector<L> &vector);

using RobotVector3 = RobotVector<3>;
using RobotVector6 = RobotVector<6>;
using RobotVector7 = RobotVector<7>;

using PRobotVector3T = PRobotVectorT<3>;
using PRobotVector6T = PRobotVectorT<6>;
using PRobotVector7T = PRobotVectorT<7>;

using CRRobotVector3T = CRRobotVectorT<3>;
using CRRobotVector6T = CRRobotVectorT<6>;
using CRRobotVector7T = CRRobotVectorT<7>;

#endif //SOPHIAR_ROBOTTYPES_H
