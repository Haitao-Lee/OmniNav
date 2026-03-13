#pragma once

#include "VisualizationUtils.h"
#include <vector>
#include <string>
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkPolyData.h>
#include <vtkSphereSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

class Landmark {
public:
    explicit Landmark(const double coordinates[3], const std::string& pointset_id, std::string orientation = "RAS");
    ~Landmark();

    void setPointset(const std::string& pointset_id);
    void setCoordinates(const double coordinates[3]);
    void setRadius(double r);
    void setColor(const double color[3]);
    void setOpacity(double opacity);
    void setVisible(int visible);
    void changeVisible();

    void setActor(vtkSmartPointer<vtkActor> actor);
    void setPrjActors(const std::vector<vtkSmartPointer<vtkActor>>& actors);
    void setPrjActor(vtkSmartPointer<vtkActor> actor, int num);

    std::string getPointset() const { return m_pointset_id; }
    std::string getOrientation() const { return m_orientation; }
    const double* getCoordinates() const { return m_coordinates; }
    double getRadius() const { return m_radius; }
    const double* getColor() const { return m_color; }
    double getOpacity() const { return m_opacity; }
    int getVisible() const { return m_visible; }

    vtkSmartPointer<vtkPolyData> getPolydata() const;
    vtkSmartPointer<vtkActor> getActor() const { return m_actor; }
    std::vector<vtkSmartPointer<vtkActor>> getPrjActors() const { return m_projectActors; }

    void refresh();

private:
    std::string m_pointset_id;
    std::string m_orientation;
    double m_coordinates[3];
    double m_color[3];
    double m_radius;
    double m_opacity;
    int m_visible;

    vtkSmartPointer<vtkSphereSource> m_source;
    vtkSmartPointer<vtkPolyDataMapper> m_mapper;
    vtkSmartPointer<vtkActor> m_actor;
    std::vector<vtkSmartPointer<vtkActor>> m_projectActors;

    void updateGeometry();
    void updateProperties();
};