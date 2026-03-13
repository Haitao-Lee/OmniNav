#ifndef MODEL3D_H
#define MODEL3D_H

#pragma once

#include <vector>
#include <string>
#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>

class Model3D {
public:
    explicit Model3D(std::string filePath, std::string orientation = "RAS");
    ~Model3D();

    bool loadFile(std::string path);

    void createActor();

    void setActor(vtkSmartPointer<vtkActor> actor);
    void setPrjActors(const std::vector<vtkSmartPointer<vtkActor>>& actors);
    void setPrjActor(vtkSmartPointer<vtkActor> actor, int num);

    void changeVisible();
    void setVisible(int visible);
    void setOpacity(double opacity);
    void setColor(double color[3]);
    void setName(std::string name);
    void setFilePath(std::string path);

    vtkSmartPointer<vtkPolyData> getPolydata() const { return m_polydata; }
    std::string getOrientation() const { return m_orientation; }
    vtkSmartPointer<vtkActor> getActor() const { return m_actor; }
    std::vector<vtkSmartPointer<vtkActor>> getPrjActors() const { return m_projectActors; }
    const double* getColor() const { return m_color; }
    double getOpacity() const { return m_opacity; }
    int getVisible() const { return m_visible; }
    std::string getName() const { return m_name; }
    std::string getPath() const { return m_path; }

    void refresh();

private:
    vtkSmartPointer<vtkPolyData> m_polydata;
    double m_color[3];
    int m_visible;
    double m_opacity;
    std::string m_name;
    std::string m_path;
    std::string m_orientation;
    
    vtkSmartPointer<vtkActor> m_actor;
    std::vector<vtkSmartPointer<vtkActor>> m_projectActors;
};

#endif