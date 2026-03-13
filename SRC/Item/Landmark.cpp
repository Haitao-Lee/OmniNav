#include "Landmark.h"
#include <vtkProperty.h>
#include <QDebug>
#include <string>

Landmark::Landmark(const double coordinates[3], const std::string& pointset_id, std::string orientation)
{
    m_pointset_id = pointset_id;
    m_orientation = orientation;
    m_visible = 1;
    m_opacity = 1.0;
    m_radius = 2.0;
    // qDebug() << "Add landmark:" << coordinates[0] << coordinates[1];
    // qDebug() << orientation.c_str();

    if (coordinates) {
        m_coordinates[0] = coordinates[0];
        m_coordinates[1] = coordinates[1];
        m_coordinates[2] = coordinates[2];
        if (m_orientation == "RAS") {
            m_coordinates[0] = -coordinates[0];
            m_coordinates[1] = -coordinates[1];
            m_coordinates[2] = coordinates[2];
        }
    } else {
        m_coordinates[0] = 0.0;
        m_coordinates[1] = 0.0;
        m_coordinates[2] = 0.0;
    }

    // qDebug() << "Add landmark:" << m_coordinates[0] << m_coordinates[1];

    m_color[0] = 1.0; 
    m_color[1] = 0.0; 
    m_color[2] = 0.0;

    m_source = vtkSmartPointer<vtkSphereSource>::New();
    m_source->SetThetaResolution(20);
    m_source->SetPhiResolution(20);

    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_mapper->SetInputConnection(m_source->GetOutputPort());

    m_actor = vtkSmartPointer<vtkActor>::New();
    m_actor->SetMapper(m_mapper);

    updateGeometry();
    updateProperties();
}

Landmark::~Landmark()
{
    m_projectActors.clear();
}

void Landmark::setPointset(const std::string& pointset_id)
{
    m_pointset_id = pointset_id;
}

void Landmark::setCoordinates(const double coordinates[3])
{
    if (!coordinates) return;

    if (m_coordinates[0] == coordinates[0] && 
        m_coordinates[1] == coordinates[1] && 
        m_coordinates[2] == coordinates[2]) {
        return;
    }

    m_coordinates[0] = coordinates[0];
    m_coordinates[1] = coordinates[1];
    m_coordinates[2] = coordinates[2];
    
    updateGeometry();
}

void Landmark::setRadius(double r)
{
    if (m_radius != r) {
        m_radius = r;
        updateGeometry();
    }
}

void Landmark::setColor(const double color[3])
{
    if (!color) return;

    m_color[0] = color[0];
    m_color[1] = color[1];
    m_color[2] = color[2];

    updateProperties();
}

void Landmark::setOpacity(double opacity)
{
    if (opacity > 1.0) opacity = 1.0;
    if (opacity < 0.0) opacity = 0.0;

    if (m_opacity != opacity) {
        m_opacity = opacity;
        updateProperties();
    }
}

void Landmark::setVisible(int visible)
{
    int newVisible = (visible != 0) ? 1 : 0;
    if (m_visible != newVisible) {
        m_visible = newVisible;
        updateProperties();
    }
}

void Landmark::changeVisible()
{
    m_visible = !m_visible;
    updateProperties();
}

void Landmark::setActor(vtkSmartPointer<vtkActor> actor)
{
    if (actor) {
        m_actor = actor;
        updateProperties();
    }
}

void Landmark::setPrjActors(const std::vector<vtkSmartPointer<vtkActor>>& actors)
{
    m_projectActors = actors;
    updateProperties();
}

void Landmark::setPrjActor(vtkSmartPointer<vtkActor> actor, int num)
{
    if (!actor) return;

    if (num >= 0 && num < static_cast<int>(m_projectActors.size())) {
        m_projectActors[num] = actor;
        actor->GetProperty()->SetColor(m_color);
        actor->GetProperty()->SetOpacity(m_opacity);
        actor->SetVisibility(m_visible);
    } 
    else if (num == static_cast<int>(m_projectActors.size())) {
        m_projectActors.push_back(actor);
        actor->GetProperty()->SetColor(m_color);
        actor->GetProperty()->SetOpacity(m_opacity);
        actor->SetVisibility(m_visible);
    }
}

vtkSmartPointer<vtkPolyData> Landmark::getPolydata() const
{
    if (m_source) {
        m_source->Update();
        return m_source->GetOutput();
    }
    return nullptr;
}

void Landmark::refresh()
{
    updateGeometry();
    updateProperties();
}

void Landmark::updateGeometry()
{
    if (m_source) {
        m_source->SetCenter(m_coordinates);
        m_source->SetRadius(m_radius);
        m_source->Update();
    }
}

void Landmark::updateProperties()
{
    if (m_actor) {
        m_actor->GetProperty()->SetColor(m_color);
        m_actor->GetProperty()->SetOpacity(m_opacity);
        m_actor->SetVisibility(m_visible);
    }

    for (auto& actor : m_projectActors) {
        if (actor) {
            actor->GetProperty()->SetColor(m_color);
            actor->GetProperty()->SetOpacity(m_opacity);
            actor->SetVisibility(m_visible);
        }
    }
}