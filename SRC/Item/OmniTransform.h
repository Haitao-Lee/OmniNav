#ifndef OMNITRANSFORM_H
#define OMNITRANSFORM_H

#include <QString>
#include <Eigen/Dense>

class OmniTransform {
public:
    // 构造函数，默认为单位矩阵
    OmniTransform(const QString& name = "", const Eigen::Matrix4d& value = Eigen::Matrix4d::Identity());

    // Setters
    void setName(const QString& name);
    void setValue(const Eigen::Matrix4d& value);

    // Getters
    QString getName() const;
    Eigen::Matrix4d getValue() const;
    
    // 获取引用的 Getter (高效读取)
    const Eigen::Matrix4d& getValueRef() const;

private:
    QString m_name;
    Eigen::Matrix4d m_value;
};

#endif // OMNITRANSFORM_H