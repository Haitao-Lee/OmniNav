#ifndef TOOL_H
#define TOOL_H

#pragma once

#include <string>
#include <vector>

#include <vtkSmartPointer.h>
#include <vtkPolyData.h>
#include <vtkActor.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkMatrix4x4.h>

class Tool {
public:
    explicit Tool(std::string romPath);
    ~Tool() = default;

    static vtkSmartPointer<vtkActor> createActor(vtkSmartPointer<vtkPolyData> polydata, 
                                                 double opacity = 1.0, 
                                                 bool visible = true, 
                                                 double color[3] = nullptr);

    void setName(const std::string& name);
    void setPath(const std::string& path);
    void setColor(double r, double g, double b);
    void setColor(double rgb[3]);
    void setOpacity(double opacity);
    void setVisible(bool visible);
    void changeVisible();
    void setRMSE(double rmse);
    
    // Geometry setters
    void setRadius(double radius);
    void setLength(double length);
    
    // [Changed] Replaced flipZ with direction string (e.g., "Z-", "X+")
    void setDirection(const std::string& direction); 

    void setHandle(int handle);
    void setPolydata(vtkSmartPointer<vtkPolyData> polydata);
    void setActor(vtkSmartPointer<vtkActor> actor);
    
    void setRawMatrix(vtkSmartPointer<vtkMatrix4x4> rawMatrix);
    void setRegistrationMatrix(vtkSmartPointer<vtkMatrix4x4> regMatrix);
    void setCalibrationMatrix(vtkSmartPointer<vtkMatrix4x4> caliMatrix);

    // Getters
    std::string getName() const { return m_name; }
    std::string getPath() const { return m_path; }
    void getColor(double color[3]) const;
    double getOpacity() const { return m_opacity; }
    bool getVisible() const { return m_visible; }
    double getRMSE() const { return m_rmse; }
    double getRadius() const { return m_radius; }
    double getLength() const { return m_length; }
    
    // [Changed] Getter for direction
    std::string getDirection() const { return m_direction; }
    
    int getHandle() const { return m_handle; }
    std::vector<vtkSmartPointer<vtkActor>> getPrjActors() const { return m_projectActors; }
    
    vtkSmartPointer<vtkPolyData> getPolydata() const { return m_polydata; }
    vtkSmartPointer<vtkActor> getActor() const { return m_actor; }

    vtkSmartPointer<vtkMatrix4x4> getFinalMatrix() const { return m_finalMatrix; }
    vtkSmartPointer<vtkMatrix4x4> getRawMatrix() const { return m_rawMatrix; }
    vtkSmartPointer<vtkMatrix4x4> getRegistrationMatrix() const { return m_regMatrix; }
    vtkSmartPointer<vtkMatrix4x4> getCalibrationMatrix() const { return m_calibrationMatrix; }
    vtkSmartPointer<vtkMatrix4x4> getMatrix() const { return m_finalMatrix; }

    vtkSmartPointer<vtkMatrix4x4> getActorMatrix() const { return m_actorMatrix; }

private:
    void updateFinalMatrix();

    std::string m_name;
    std::string m_path;
    double m_color[3];
    double m_opacity;
    bool m_visible;
    double m_rmse;

    double m_radius;
    double m_length;
    
    // [Changed] Store direction string
    std::string m_direction; 
    
    int m_handle;

    vtkSmartPointer<vtkPolyData> m_polydata;
    vtkSmartPointer<vtkActor> m_actor;
    
    vtkSmartPointer<vtkMatrix4x4> m_rawMatrix;        
    vtkSmartPointer<vtkMatrix4x4> m_regMatrix;        
    vtkSmartPointer<vtkMatrix4x4> m_calibrationMatrix; 
    vtkSmartPointer<vtkMatrix4x4> m_finalMatrix; 
    vtkSmartPointer<vtkMatrix4x4> m_actorMatrix;     

    std::vector<vtkSmartPointer<vtkActor>> m_projectActors;
};

#endif