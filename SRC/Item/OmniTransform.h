#ifndef OMNITRANSFORM_H
#define OMNITRANSFORM_H

#include <QString>
#include <Eigen/Dense>

class OmniTransform {
public:
    // Constructor; defaults to the identity matrix.
    OmniTransform(const QString& name = "", const Eigen::Matrix4d& value = Eigen::Matrix4d::Identity());

    // Setters
    void setName(const QString& name);
    void setValue(const Eigen::Matrix4d& value);

    // Getters
    QString getName() const;
    Eigen::Matrix4d getValue() const;
    
    // Getter that returns a reference (efficient access).
    const Eigen::Matrix4d& getValueRef() const;

private:
    QString m_name;
    Eigen::Matrix4d m_value;
};

#endif // OMNITRANSFORM_H
