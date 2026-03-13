#include "Model3D.h"
#include <algorithm>
#include <QFileInfo>
#include <QString>
#include <vtkSTLReader.h>
#include <vtkOBJReader.h>
#include <vtkPLYReader.h>
#include <vtkPolyDataReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkBYUReader.h>
#include <vtkSmartPointer.h>
#include <vtkTransform.h>
#include <vtkTransformPolyDataFilter.h>


Model3D::Model3D(std::string filePath, std::string orientation)
    : m_path(filePath), m_orientation(orientation)
{
    m_visible = 1;
    m_opacity = 1.0;
    m_color[0] = 0.0;
    m_color[1] = 0.0; 
    m_color[2] = 0.5;

    QFileInfo fileInfo(QString::fromStdString(filePath));
    m_name = fileInfo.completeBaseName().toStdString();

    if (loadFile(m_path)) {
        createActor();
    } else {
        m_polydata = nullptr;
        m_actor = nullptr;
    }
}

Model3D::~Model3D()
{
}

bool Model3D::loadFile(std::string path)
{
    QString qPath = QString::fromStdString(path);
    QFileInfo fileInfo(qPath);
    QString extension = fileInfo.suffix().toLower();
    
    m_polydata = nullptr;

    if (extension == "stl") {
        vtkSmartPointer<vtkSTLReader> reader = vtkSmartPointer<vtkSTLReader>::New();
        reader->SetFileName(path.c_str());
        reader->Update();
        m_polydata = reader->GetOutput();
    } 
    else if (extension == "obj") {
        vtkSmartPointer<vtkOBJReader> reader = vtkSmartPointer<vtkOBJReader>::New();
        reader->SetFileName(path.c_str());
        reader->Update();
        m_polydata = reader->GetOutput();
    }
    else if (extension == "ply") {
        vtkSmartPointer<vtkPLYReader> reader = vtkSmartPointer<vtkPLYReader>::New();
        reader->SetFileName(path.c_str());
        reader->Update();
        m_polydata = reader->GetOutput();
    }
    else if (extension == "vtk") {
        vtkSmartPointer<vtkPolyDataReader> reader = vtkSmartPointer<vtkPolyDataReader>::New();
        reader->SetFileName(path.c_str());
        reader->Update();
        m_polydata = reader->GetOutput();
    }
    else if (extension == "vtp") {
        vtkSmartPointer<vtkXMLPolyDataReader> reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
        reader->SetFileName(path.c_str());
        reader->Update();
        m_polydata = reader->GetOutput();
    }
    else if (extension == "g") {
        vtkSmartPointer<vtkBYUReader> reader = vtkSmartPointer<vtkBYUReader>::New();
        reader->SetGeometryFileName(path.c_str());
        reader->Update();
        m_polydata = reader->GetOutput();
    }
    if (m_polydata && m_orientation == "RAS")
    {
        vtkSmartPointer<vtkTransform> transform = vtkSmartPointer<vtkTransform>::New();
        transform->RotateZ(180.0);

        vtkSmartPointer<vtkTransformPolyDataFilter> filter = vtkSmartPointer<vtkTransformPolyDataFilter>::New();
        filter->SetInputData(m_polydata);
        filter->SetTransform(transform);
        filter->Update();

        m_polydata->DeepCopy(filter->GetOutput());
    }
        
    return (m_polydata != nullptr && m_polydata->GetNumberOfPoints() > 0);
}

void Model3D::createActor()
{
    if (!m_polydata) return;

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(m_polydata);

    m_actor = vtkSmartPointer<vtkActor>::New();
    m_actor->SetMapper(mapper);
    
    if (m_color) {
        m_actor->GetProperty()->SetColor(m_color);
    } else {
        m_actor->GetProperty()->SetColor(1.0, 1.0, 1.0);
    }
    
    m_actor->GetProperty()->SetOpacity(m_opacity);
    m_actor->GetProperty()->ShadingOn();
    m_actor->SetVisibility(m_visible);
}

void Model3D::setActor(vtkSmartPointer<vtkActor> actor)
{
    m_actor = actor;
    if (m_actor) {
        m_actor->GetProperty()->SetColor(m_color);
        m_actor->GetProperty()->SetOpacity(m_opacity);
        m_actor->SetVisibility(m_visible);
    }
}

void Model3D::setPrjActors(const std::vector<vtkSmartPointer<vtkActor>>& actors)
{
    for (const auto& actor : actors) {
        if (actor) {
            actor->GetProperty()->SetColor(m_color);
            actor->GetProperty()->SetOpacity(m_opacity);
            actor->SetVisibility(m_visible);
            m_projectActors.push_back(actor);
        }
    }
}

void Model3D::setPrjActor(vtkSmartPointer<vtkActor> actor, int num)
{
    if (!actor) return;
    
    actor->GetProperty()->SetColor(m_color);
    actor->GetProperty()->SetOpacity(m_opacity);
    actor->SetVisibility(m_visible);
    
    if (num >= 0 && num < static_cast<int>(m_projectActors.size())) {
        m_projectActors[num] = actor;
    }
}

void Model3D::changeVisible()
{
    m_visible = 1 - m_visible;
    if (m_actor) {
        m_actor->SetVisibility(m_visible);
    }
    for (auto& actor : m_projectActors) {
        if (actor) actor->SetVisibility(m_visible);
    }
}

void Model3D::setVisible(int visible)
{
    m_visible = (visible != 0) ? 1 : 0;
    if (m_actor) {
        m_actor->SetVisibility(m_visible);
    }
    for (auto& actor : m_projectActors) {
        if (actor) actor->SetVisibility(m_visible);
    }
}

void Model3D::setOpacity(double opacity)
{
    if (opacity > 1.0) opacity = 1.0;
    if (opacity < 0.0) opacity = 0.0;
    
    m_opacity = opacity;
    
    if (m_actor) {
        m_actor->GetProperty()->SetOpacity(m_opacity);
        m_actor->SetVisibility(m_visible);
    }
    for (auto& actor : m_projectActors) {
        if (actor) {
            actor->GetProperty()->SetOpacity(m_opacity);
            actor->SetVisibility(m_visible);
        }
    }
}

void Model3D::setColor(double color[3])
{
    m_color[0] = color[0];
    m_color[1] = color[1];
    m_color[2] = color[2];

    if (m_actor) {
        m_actor->GetProperty()->SetColor(m_color);
    }
    for (auto& actor : m_projectActors) {
        if (actor) actor->GetProperty()->SetColor(m_color);
    }
}

void Model3D::setName(std::string name)
{
    m_name = name;
}

void Model3D::setFilePath(std::string path)
{
    m_path = path;
}

void Model3D::refresh()
{
    if (m_actor) {
        m_actor->GetProperty()->SetColor(m_color);
        m_actor->GetProperty()->SetOpacity(m_opacity);
    }
    for (auto& actor : m_projectActors) {
        if (actor) {
            actor->GetProperty()->SetColor(m_color);
            actor->GetProperty()->SetOpacity(m_opacity);
        }
    }
}