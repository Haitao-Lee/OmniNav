#include "Tool.h"
#include "VisualizationUtils.h"
#include <vtkTransform.h>
#include <vtksys/SystemTools.hxx>
#include <vtkPolyDataMapper.h>
#include <QString> // Needed for conversion to VisualizationUtils
#include <QDebug>

Tool::Tool(std::string romPath)
    : m_path(romPath), 
      m_opacity(1.0), 
      m_visible(false),
      m_rmse(0.0),
      m_radius(5.0),
      m_length(250.0),
      m_direction("Z+"), // Default direction
      m_handle(-1)
{
    m_name = vtksys::SystemTools::GetFilenameWithoutLastExtension(m_path);

    m_color[0] = 1.0; m_color[1] = 1.0; m_color[2] = 0.0;

    // Generate geometry with default direction
    m_polydata = VisualizationUtils::getNeedlePolyData(m_radius, m_length, QString::fromStdString(m_direction));

    m_rawMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    m_rawMatrix->Identity();

    m_regMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    m_regMatrix->Identity();
    
    m_calibrationMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    m_calibrationMatrix->Identity();

    m_finalMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    m_finalMatrix->Identity();

    m_actorMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
    m_actorMatrix->Identity();

    m_actor = createActor(m_polydata, m_opacity, m_visible, m_color);
    if (m_actor) {
        m_actor->SetUserMatrix(m_actorMatrix);
    }
}

vtkSmartPointer<vtkActor> Tool::createActor(vtkSmartPointer<vtkPolyData> polydata, 
                                            double opacity, bool visible, double color[3]) {
    if (!polydata) return nullptr;

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(polydata);

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    
    if (color) actor->GetProperty()->SetColor(color[0], color[1], color[2]);
    else actor->GetProperty()->SetColor(1.0, 1.0, 1.0); 
    
    actor->GetProperty()->SetOpacity(opacity);
    actor->SetVisibility(visible ? 1 : 0);

    return actor;
}

void Tool::changeVisible() {
    setVisible(!m_visible);
}

void Tool::setName(const std::string& name) {
    m_name = name;
}

void Tool::setPath(const std::string& path) {
    m_path = path;
}

void Tool::setColor(double r, double g, double b) {
    m_color[0] = r; m_color[1] = g; m_color[2] = b;
    if (m_actor) m_actor->GetProperty()->SetColor(m_color);
}

void Tool::setColor(double rgb[3]) {
    if (!rgb) return;
    setColor(rgb[0], rgb[1], rgb[2]);
}

void Tool::setOpacity(double opacity) {
    if (opacity > 1.0) opacity = 1.0;
    if (opacity < 0.0) opacity = 0.0;
    m_opacity = opacity;
    if (m_actor) m_actor->GetProperty()->SetOpacity(m_opacity);
}

void Tool::setVisible(bool visible) {
    m_visible = visible;
    if (m_actor) m_actor->SetVisibility(m_visible ? 1 : 0);
}

void Tool::setRMSE(double rmse) {
    m_rmse = rmse;
}

void Tool::setRadius(double radius) {
    if (std::abs(m_radius - radius) < 1e-6) return;
    m_radius = radius;
    // Update geometry with current direction
    m_polydata = VisualizationUtils::getNeedlePolyData(m_radius, m_length, QString::fromStdString(m_direction));
    if (m_actor) {
        auto mapper = vtkPolyDataMapper::SafeDownCast(m_actor->GetMapper());
        if (mapper) mapper->SetInputData(m_polydata);
    }
}

void Tool::setLength(double length) {
    if (std::abs(m_length - length) < 1e-6) return;
    m_length = length;
    // Update geometry with current direction
    m_polydata = VisualizationUtils::getNeedlePolyData(m_radius, m_length, QString::fromStdString(m_direction));
    if (m_actor) {
        auto mapper = vtkPolyDataMapper::SafeDownCast(m_actor->GetMapper());
        if (mapper) mapper->SetInputData(m_polydata);
    }
}

// [New] Implementation for setting direction
void Tool::setDirection(const std::string& direction) {
    if (m_direction == direction) return;
    m_direction = direction;
    
    // Regenerate polydata based on new direction
    m_polydata = VisualizationUtils::getNeedlePolyData(m_radius, m_length, QString::fromStdString(m_direction));
    
    if (m_actor) {
        auto mapper = vtkPolyDataMapper::SafeDownCast(m_actor->GetMapper());
        if (mapper) mapper->SetInputData(m_polydata);
    }
}

void Tool::setHandle(int handle) {
    m_handle = handle;
}

void Tool::setPolydata(vtkSmartPointer<vtkPolyData> polydata) {
    if (!polydata) return;
    m_polydata = polydata;
    if (m_actor) {
        auto mapper = vtkPolyDataMapper::SafeDownCast(m_actor->GetMapper());
        if (!mapper) {
            mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
            m_actor->SetMapper(mapper);
        }
        mapper->SetInputData(m_polydata);
    }
    updateFinalMatrix();
}

void Tool::setActor(vtkSmartPointer<vtkActor> actor) {
    if (!actor) return;
    m_actor = actor;
    m_actor->GetProperty()->SetOpacity(m_opacity);
    m_actor->GetProperty()->SetColor(m_color);
    m_actor->SetVisibility(m_visible ? 1 : 0);
    m_actor->SetUserMatrix(m_finalMatrix); 
}

void Tool::setRawMatrix(vtkSmartPointer<vtkMatrix4x4> rawMatrix) {
    if (rawMatrix) {
        m_rawMatrix->DeepCopy(rawMatrix);
        updateFinalMatrix();
    }
}

void Tool::setRegistrationMatrix(vtkSmartPointer<vtkMatrix4x4> regMatrix) {
    if (regMatrix) {
        m_regMatrix->DeepCopy(regMatrix);
        updateFinalMatrix();
    }
    // // Debug output kept as requested
    // qDebug() << "========================================";
    // qDebug() << "Raw  XYZ:" << m_rawMatrix->GetElement(0, 3) << m_rawMatrix->GetElement(1, 3) << m_rawMatrix->GetElement(2, 3);
    // qDebug() << "Cali XYZ:" << m_calibrationMatrix->GetElement(0, 3) << m_calibrationMatrix->GetElement(1, 3) << m_calibrationMatrix->GetElement(2, 3);
    // qDebug() << "Reg  XYZ:" << m_regMatrix->GetElement(0, 3) << m_regMatrix->GetElement(1, 3) << m_regMatrix->GetElement(2, 3);
    // qDebug() << "Final XYZ:" << m_finalMatrix->GetElement(0, 3) << m_finalMatrix->GetElement(1, 3) << m_finalMatrix->GetElement(2, 3);
    // qDebug() << "========================================";
}

void Tool::setCalibrationMatrix(vtkSmartPointer<vtkMatrix4x4> caliMatrix) {
    if (caliMatrix) {
        m_calibrationMatrix->DeepCopy(caliMatrix);
        updateFinalMatrix();
    }
    // qDebug() << "========================================";
    // qDebug() << "Raw  XYZ:" << m_rawMatrix->GetElement(0, 3) << m_rawMatrix->GetElement(1, 3) << m_rawMatrix->GetElement(2, 3);
    // qDebug() << "Cali XYZ:" << m_calibrationMatrix->GetElement(0, 3) << m_calibrationMatrix->GetElement(1, 3) << m_calibrationMatrix->GetElement(2, 3);
    // qDebug() << "Reg  XYZ:" << m_regMatrix->GetElement(0, 3) << m_regMatrix->GetElement(1, 3) << m_regMatrix->GetElement(2, 3);
    // qDebug() << "Final XYZ:" << m_finalMatrix->GetElement(0, 3) << m_finalMatrix->GetElement(1, 3) << m_finalMatrix->GetElement(2, 3);
    // qDebug() << "========================================";
}

void Tool::updateFinalMatrix() {
    // ---------------------------------------------------------
    // 1. Update m_finalMatrix (PURE DATA)
    // Formula: Final = Reg * Raw * Cali
    // ---------------------------------------------------------
    vtkSmartPointer<vtkMatrix4x4> tempMat = vtkSmartPointer<vtkMatrix4x4>::New();
    vtkMatrix4x4::Multiply4x4(m_rawMatrix, m_calibrationMatrix, tempMat);
    vtkMatrix4x4::Multiply4x4(m_regMatrix, tempMat, m_finalMatrix);
    
    m_finalMatrix->Modified(); // Notify tracking data consumers

    // ---------------------------------------------------------
    // 2. Update m_actorMatrix (VISUALIZATION ONLY)
    // Formula: ActorMat = Final * Correction
    // ---------------------------------------------------------
    
    // Calculate Correction Transform based on direction
    vtkSmartPointer<vtkTransform> correctionTransform = vtkSmartPointer<vtkTransform>::New();
    
    // Assumption: The PolyData is built along +Z or -Z default.
    // We rotate the local geometry to align with tracking Z-axis.
    if (m_direction == "Z+") {
        correctionTransform->RotateX(180);
    }
    else if (m_direction == "Z-") {
        correctionTransform->RotateX(0);
    }
    else if (m_direction == "X+") {
        correctionTransform->RotateY(-90);
    }
    else if (m_direction == "X-") {
        correctionTransform->RotateY(90);
    }
    else if (m_direction == "Y+") {
        correctionTransform->RotateX(90);
    }
    else if (m_direction == "Y-") {
        correctionTransform->RotateX(-90);
    }
    correctionTransform->Update();

    // Combine: Visual = FinalMatrix (Pose) * Correction (Rotation)
    vtkMatrix4x4::Multiply4x4(m_finalMatrix, correctionTransform->GetMatrix(), m_actorMatrix);
    
    // 3. Notify Actor
    m_actorMatrix->Modified();
    if (m_actor) m_actor->Modified();
}

void Tool::getColor(double color[3]) const {
    color[0] = m_color[0];
    color[1] = m_color[1];
    color[2] = m_color[2];
}