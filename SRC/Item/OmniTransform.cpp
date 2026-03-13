#include "OmniTransform.h"

OmniTransform::OmniTransform(const QString& name, const Eigen::Matrix4d& value)
    : m_name(name), m_value(value) 
{
}

void OmniTransform::setName(const QString& name) {
    m_name = name;
}

void OmniTransform::setValue(const Eigen::Matrix4d& value) {
    m_value = value;
}

QString OmniTransform::getName() const {
    return m_name;
}

Eigen::Matrix4d OmniTransform::getValue() const {
    return m_value;
}

const Eigen::Matrix4d& OmniTransform::getValueRef() const {
    return m_value;
}