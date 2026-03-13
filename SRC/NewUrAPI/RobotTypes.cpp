#include "RobotTypes.h"
#include "static_block.hpp"

#include <QMetaType>

#include <format>
#include <spdlog/spdlog.h>

static_block {
    qRegisterMetaType<RobotVector3>("RobotVector3");
    qRegisterMetaType<RobotVector6>("RobotVector6");
    qRegisterMetaType<RobotVector7>("RobotVector7");
    qRegisterMetaType<PRobotVector3T>("PRobotVector3T");
    qRegisterMetaType<PRobotVector6T>("PRobotVector6T");
    qRegisterMetaType<PRobotVector7T>("PRobotVector7T");
};

template<size_t L>
bool ReadRobotVector(EasyDataIO &dio, RobotVector<L> &vector) {
    for (size_t i = 0; i < L; ++i) {
        if (!dio.Read(vector.at(i))) {
            SPDLOG_ERROR("Failed to read robot vector.");
            return false;
        }
    }
    return true;
}

template bool ReadRobotVector<3>(EasyDataIO &dio, RobotVector3 &vector);

template bool ReadRobotVector<6>(EasyDataIO &dio, RobotVector6 &vector);

template bool ReadRobotVector<7>(EasyDataIO &dio, RobotVector7 &vector);

template<size_t L>
bool WriteRobotVector(EasyDataIO &dio, const RobotVector<L> &vector) {
    for (size_t i = 0; i < L; ++i) {
        if (!dio.Write(vector.at(i))) {
            SPDLOG_ERROR("Failed to write robot vector.");
            return false;
        }
    }
    return true;
}

template bool WriteRobotVector<3>(EasyDataIO &dio, const RobotVector3 &vector);

template bool WriteRobotVector<6>(EasyDataIO &dio, const RobotVector6 &vector);

template bool WriteRobotVector<7>(EasyDataIO &dio, const RobotVector7 &vector);