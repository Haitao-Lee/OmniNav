#ifndef IMAGE_H
#define IMAGE_H

#pragma once

#include <vector>
#include <string>
#include <tuple> 

#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkMatrix4x4.h>
#include <vtkProp.h>
#include <vtkLookupTable.h>
#include <vtkImageReader2.h> 
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkImageReslice.h>
#include <vtkImageMapToColors.h>
#include <vtkImageActor.h>
#include <vtkVolume.h>
#include <vtkVolumeProperty.h>
#include <vtkColorTransferFunction.h>
#include <vtkPiecewiseFunction.h>
#include <vtkImageCast.h>
#include <vtkImageFlip.h>
#include <vtkMedicalImageProperties.h>

class Image {
public:
    explicit Image(std::string filePath, std::string m_orientation = "RAS");
    ~Image();

    bool loadFile();

    void createActors(vtkSmartPointer<vtkLookupTable> lut2d, 
                      vtkSmartPointer<vtkColorTransferFunction> ctf3d, 
                      vtkSmartPointer<vtkPiecewiseFunction> pwf3d);
    
    void adjustActors(double current_center[3]);

    vtkSmartPointer<vtkImageData> getImageData() const { return m_imageData; }
    std::vector<vtkSmartPointer<vtkProp>> getActors() const { return m_actors; }
    
    std::string getName() const { return m_patientName; }
    std::string getOrientation() const { return m_orientation; }
    std::string getAge() const { return m_patientAge; }
    std::string getPath() const { return m_path; }
    const int* getResolution() const { return m_dimensions; }
    const double* getSpacing() const { return m_spacing; }
    
private:
    std::string m_path;
    std::string m_orientation;
    std::string m_patientName = "unknown";
    std::string m_patientAge = "unknown";
    
    double m_spacing[3];
    int m_dimensions[3];
    double m_origin[3];

    vtkSmartPointer<vtkImageData> m_imageData;
    std::vector<vtkSmartPointer<vtkProp>> m_actors; 
    std::vector<vtkSmartPointer<vtkImageReslice>> m_reslices;
    std::vector<vtkSmartPointer<vtkImageMapToColors>> m_colormaps;
    vtkSmartPointer<vtkVolumeProperty> m_volumeProperty;

    void getOrthogonalMatrix(int direction, vtkMatrix4x4* matrix);
};

#endif