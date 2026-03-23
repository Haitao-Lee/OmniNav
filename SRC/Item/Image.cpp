#include "Image.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QStringList>
#include <QDebug>
#include <cmath>

#include <vtkDICOMImageReader.h>
#include <vtkNIFTIImageReader.h>
#include <vtkNrrdReader.h>
#include <vtkMetaImageReader.h>
#include <vtkFixedPointVolumeRayCastMapper.h> 
#include <vtkGPUVolumeRayCastMapper.h>
#include <vtkSmartVolumeMapper.h>
#include <vtkImageMapper3D.h> 
#include <vtkImageShiftScale.h> 
#include <vtkImageSlice.h>
#include <vtkImageSliceMapper.h>

#include <itkImage.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkImageSeriesReader.h>
#include <itkMetaDataObject.h>


Image::Image(std::string filePath, std::string m_orientation)
    : m_path(filePath), m_orientation(m_orientation)
{
    m_spacing[0] = 1.0; m_spacing[1] = 1.0; m_spacing[2] = 1.0;
    m_dimensions[0] = 0; m_dimensions[1] = 0; m_dimensions[2] = 0;
    m_origin[0] = 0.0; m_origin[1] = 0.0; m_origin[2] = 0.0;

    if (!loadFile()) {
        m_imageData = nullptr;
    }
}

Image::~Image()
{
}

// bool Image::loadFile()
// {
//     QString qPath = QString::fromStdString(m_path);
//     qPath.replace("\\", "/");
//     QFileInfo fileInfo(qPath);

//     if (!fileInfo.exists()) return false;

//     vtkSmartPointer<vtkImageReader2> reader = nullptr;
//     vtkSmartPointer<vtkDICOMImageReader> dcmReader = nullptr;

//     // 1. Initialize the reader (keep as-is).
//     if (fileInfo.isDir())
//     {
//         QDir dir(qPath);
//         QStringList filters; filters << "*.dcm";
//         if (dir.entryInfoList(filters, QDir::Files).isEmpty()) return false;
//         dcmReader = vtkSmartPointer<vtkDICOMImageReader>::New();
//         dcmReader->SetDirectoryName(qPath.toStdString().c_str());
//         reader = dcmReader;
//     }
//     else if (fileInfo.isFile())
//     {
//         std::string suffix = fileInfo.suffix().toLower().toStdString();
//         std::string completeSuffix = fileInfo.completeSuffix().toLower().toStdString();

//         if (suffix == "dcm") {
//             dcmReader = vtkSmartPointer<vtkDICOMImageReader>::New();
//             dcmReader->SetFileName(qPath.toStdString().c_str());
//             reader = dcmReader;
//         } else if (suffix == "nii" || completeSuffix == "nii.gz") {
//             auto niiReader = vtkSmartPointer<vtkNIFTIImageReader>::New();
//             niiReader->SetFileName(qPath.toStdString().c_str());
//             reader = niiReader;
//         } else if (suffix == "nrrd") {
//             auto nrrdReader = vtkSmartPointer<vtkNrrdReader>::New();
//             nrrdReader->SetFileName(qPath.toStdString().c_str());
//             reader = nrrdReader;
//         } else if (suffix == "mhd" || suffix == "mha") {
//             auto mhdReader = vtkSmartPointer<vtkMetaImageReader>::New();
//             mhdReader->SetFileName(qPath.toStdString().c_str());
//             reader = mhdReader;
//         }
//     }

//     if (!reader) return false;

//     QCoreApplication::processEvents();
//     reader->Update();
//     QCoreApplication::processEvents();

//     vtkSmartPointer<vtkImageData> tempImage = reader->GetOutput();

//     double range[2];
//     // Get the min/max scalar values from the image array.
//     tempImage->GetScalarRange(range);

//     // Print to the debug console.
//     qDebug() << "------------------------------------------";
//     qDebug() << "m_imageData Scalar Range:";
//     qDebug() << "Min Value:" << range[0];
//     qDebug() << "Max Value:" << range[1];
//     qDebug() << "Data Type:" << tempImage->GetScalarTypeAsString();
//     qDebug() << "------------------------------------------";

//     if (!tempImage) return false;

//     if (dcmReader) {
//         const char* pName = dcmReader->GetPatientName();
//         if (pName) m_patientName = std::string(pName);
//     }

//     // --- 2. Coordinate normalization logic (Slicer RAS consistency) ---

//     tempImage->GetOrigin(m_origin);
//     tempImage->GetSpacing(m_spacing);
//     tempImage->GetDimensions(m_dimensions);
//     int extent[6];
//     tempImage->GetExtent(extent);

//     if (dcmReader)
//     {
//         double newOriginX = m_origin[0];
//         double newOriginY = m_origin[1];
//         double newOriginZ = m_origin[2];

//         // Slicer uses RAS; DICOM uses LPS.
//         // 3. Physically flip pixels.
//         // Flip the X axis (L->R).
//         vtkSmartPointer<vtkImageFlip> flipX = vtkSmartPointer<vtkImageFlip>::New();
//         flipX->SetInputData(tempImage);
//         flipX->SetFilteredAxis(0); 
//         flipX->Update();
//         tempImage = flipX->GetOutput();
//         newOriginX = m_origin[0] + (m_dimensions[0] - 1) * m_spacing[0];

//         // Flip the Y axis (P->A). 
//         // vtkSmartPointer<vtkImageFlip> flipY = vtkSmartPointer<vtkImageFlip>::New();
//         // flipY->SetInputData(tempImage);
//         // flipY->SetFilteredAxis(1); 
//         // flipY->Update();
//         // tempImage = flipY->GetOutput();
//         // newOriginY = m_origin[1] + (m_dimensions[1] - 1) * m_spacing[1];

//         // Flip the Z axis. 
//         vtkSmartPointer<vtkImageFlip> flipZ = vtkSmartPointer<vtkImageFlip>::New();
//         flipZ->SetInputData(tempImage);
//         flipZ->SetFilteredAxis(2); 
//         flipZ->Update();
//         tempImage = flipZ->GetOutput();
//         newOriginZ = m_origin[2] + (m_dimensions[2] - 1) * m_spacing[2];

//         // 4. Apply the corrected origin back to the image.
//         // Note: SetDirectionMatrix is unavailable in older versions,
//         // so we must preserve consistency via Origin and physical pixel flips.
//         m_origin[0] = newOriginX;
//         m_origin[1] = newOriginY;
//         m_origin[2] = newOriginZ;
//         tempImage->SetOrigin(m_origin);
//         }

//     m_imageData = tempImage;

//     return true;
// }


bool Image::loadFile()
{
    QString qPath = QString::fromStdString(m_path);
    qPath.replace("\\", "/");
    QFileInfo fileInfo(qPath);

    if (!fileInfo.exists()) return false;

    vtkSmartPointer<vtkImageData> tempImage = nullptr;
    bool isDcm = false;

    if (fileInfo.isDir() || fileInfo.suffix().toLower() == "dcm")
    {
        isDcm = true;
        typedef itk::Image<short, 3> ITKImageType;
        typedef itk::ImageSeriesReader<ITKImageType> ITKReaderType;

        auto nameGenerator = itk::GDCMSeriesFileNames::New();
        nameGenerator->SetDirectory(qPath.toLocal8Bit().constData());
        
        auto fileNames = nameGenerator->GetInputFileNames();
        if (fileNames.empty()) return false;

        auto itkReader = ITKReaderType::New();
        auto gdcmIO = itk::GDCMImageIO::New();
        itkReader->SetImageIO(gdcmIO);
        itkReader->SetFileNames(fileNames);

        try {
            itkReader->Update();
            
            const itk::MetaDataDictionary& dict = *(itkReader->GetMetaDataDictionaryArray()->at(0));
            std::string pName;
            itk::ExposeMetaData<std::string>(dict, "0010|0010", pName);
            m_patientName = pName;

            std::string pAge;
            if (itk::ExposeMetaData<std::string>(dict, "0010|1010", pAge)) {
                m_patientAge = pAge;
            } else {
                m_patientAge = "Unknown";
            }

            // Manually bridge ITK to VTK (replaces ImageToVTKImageFilter).
            ITKImageType::Pointer itkImage = itkReader->GetOutput();
            ITKImageType::RegionType region = itkImage->GetLargestPossibleRegion();
            ITKImageType::SizeType size = region.GetSize();
            ITKImageType::SpacingType spacing = itkImage->GetSpacing();
            ITKImageType::PointType origin = itkImage->GetOrigin();

            tempImage = vtkSmartPointer<vtkImageData>::New();
            tempImage->SetDimensions(size[0], size[1], size[2]);
            tempImage->SetSpacing(spacing[0], spacing[1], spacing[2]);
            tempImage->SetOrigin(origin[0], origin[1], origin[2]);
            tempImage->AllocateScalars(VTK_SHORT, 1);

            void* vtkPtr = tempImage->GetScalarPointer();
            const void* itkPtr = itkImage->GetBufferPointer();
            size_t numBytes = size[0] * size[1] * size[2] * sizeof(short);
            std::memcpy(vtkPtr, itkPtr, numBytes);

        } catch (...) { return false; }
    }
    else
    {
        vtkSmartPointer<vtkImageReader2> vtkReader = nullptr;
        std::string suffix = fileInfo.suffix().toLower().toStdString();
        std::string completeSuffix = fileInfo.completeSuffix().toLower().toStdString();

        if (suffix == "nii" || completeSuffix == "nii.gz") {
            auto niiReader = vtkSmartPointer<vtkNIFTIImageReader>::New();
            niiReader->SetFileName(qPath.toLocal8Bit().constData());
            vtkReader = niiReader;
        } else if (suffix == "nrrd") {
            auto nrrdReader = vtkSmartPointer<vtkNrrdReader>::New();
            nrrdReader->SetFileName(qPath.toLocal8Bit().constData());
            vtkReader = nrrdReader;
        } else if (suffix == "mhd" || suffix == "mha") {
            auto mhdReader = vtkSmartPointer<vtkMetaImageReader>::New();
            mhdReader->SetFileName(qPath.toLocal8Bit().constData());
            vtkReader = mhdReader;
        }

        if (vtkReader) {
            vtkReader->Update();
            tempImage = vtkReader->GetOutput();
        }
    }

    if (!tempImage || tempImage->GetNumberOfPoints() == 0) return false;

    // --- Debug output (verify values were read) ---
    double range[2];
    tempImage->GetScalarRange(range);
    qDebug() << "------------------------------------------";
    qDebug() << "Raw Image Scalar Range:" << range[0] << "to" << range[1];
    qDebug() << "Data Type:" << tempImage->GetScalarTypeAsString();
    qDebug() << "------------------------------------------";

    // --- 2. Coordinate normalization logic (Slicer RAS consistency) ---
    tempImage->GetOrigin(m_origin);
    tempImage->GetSpacing(m_spacing);
    tempImage->GetDimensions(m_dimensions);


    if (m_orientation == "RAS")
    {
        // Get the original parameters.
        int dims[3];
        double spacing[3];
        double origin[3];
        tempImage->GetDimensions(dims);
        tempImage->GetSpacing(spacing);
        tempImage->GetOrigin(origin);

        // ==========================================================
        // Step 1: Flip the data content (equivalent to a 180-degree pixel matrix rotation).
        // ==========================================================
        
        // Flip X.
        vtkSmartPointer<vtkImageFlip> flipX = vtkSmartPointer<vtkImageFlip>::New();
        flipX->SetInputData(tempImage);
        flipX->SetFilteredAxis(0);
        flipX->Update();
        vtkSmartPointer<vtkImageData> step1 = flipX->GetOutput();

        // Flip Y.
        vtkSmartPointer<vtkImageFlip> flipY = vtkSmartPointer<vtkImageFlip>::New();
        flipY->SetInputData(step1);
        flipY->SetFilteredAxis(1);
        flipY->Update();
        
        // Get the flipped image data.
        vtkSmartPointer<vtkImageData> finalImage = flipY->GetOutput();

        // ==========================================================
        // Step 2: Compute the new origin (key step).
        // Only with this setting does the image truly rotate around the world origin (0, 0, 0).
        // ==========================================================
        
        // Compute the old image "max corner" coordinates (Top-Right-Back).
        double maxBounds[3];
        maxBounds[0] = origin[0] + (dims[0] - 1) * spacing[0];
        maxBounds[1] = origin[1] + (dims[1] - 1) * spacing[1];
        maxBounds[2] = origin[2]; // If Z does not move, keep it unchanged.

        // New origin = negative of the old max corner (because x' = -x, y' = -y).
        double newOrigin[3];
        newOrigin[0] = -maxBounds[0];
        newOrigin[1] = -maxBounds[1];
        newOrigin[2] = origin[2]; // Z typically stays unchanged after a 180-degree rotation unless you rotate around the spatial center.

        // Apply the new origin to the image data.
        finalImage->SetOrigin(newOrigin);
        tempImage = finalImage;
    }

    m_imageData = tempImage;
    return true;
}


void Image::createActors(vtkSmartPointer<vtkLookupTable> lut2d, 
                         vtkSmartPointer<vtkColorTransferFunction> ctf3d, 
                         vtkSmartPointer<vtkPiecewiseFunction> pwf3d)
{
    m_reslices.clear();
    m_colormaps.clear();
    m_actors.clear();

    if (!m_imageData) return;

    double origin[3];
    double spacing[3];
    int extent[6];
    
    m_imageData->GetOrigin(origin);
    m_imageData->GetSpacing(spacing);
    m_imageData->GetExtent(extent);

    double center[3] = {0.0, 0.0, 0.0};
    center[0] = origin[0] + 0.5 * spacing[0] * (extent[0] + extent[1]);
    center[1] = origin[1] + 0.5 * spacing[1] * (extent[2] + extent[3]);
    center[2] = origin[2] + 0.5 * spacing[2] * (extent[4] + extent[5]);

    for (int i = 0; i < 3; i++)
    {
        vtkSmartPointer<vtkMatrix4x4> resliceAxes = vtkSmartPointer<vtkMatrix4x4>::New();
        getOrthogonalMatrix(i, resliceAxes);
        
        resliceAxes->SetElement(0, 3, center[0]);
        resliceAxes->SetElement(1, 3, center[1]);
        resliceAxes->SetElement(2, 3, center[2]);

        vtkSmartPointer<vtkImageReslice> reslice = vtkSmartPointer<vtkImageReslice>::New();
        reslice->SetInputData(m_imageData);
        reslice->SetOutputDimensionality(2);
        reslice->SetResliceAxes(resliceAxes);
        reslice->SetInterpolationModeToCubic();

        vtkSmartPointer<vtkImageMapToColors> colorMap = vtkSmartPointer<vtkImageMapToColors>::New();
        colorMap->SetLookupTable(lut2d);
        colorMap->SetInputConnection(reslice->GetOutputPort());
        colorMap->Update();

        vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
        actor->GetMapper()->SetInputConnection(colorMap->GetOutputPort());

        m_reslices.push_back(reslice);
        m_colormaps.push_back(colorMap);
        m_actors.push_back(actor);
    }


    // vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
    // // vtkSmartPointer<vtkSmartVolumeMapper> volumeMapper = vtkSmartPointer<vtkSmartVolumeMapper>::New();
    // volumeMapper->SetInputData(m_imageData);
    // volumeMapper->SetSampleDistance(volumeMapper->GetSampleDistance() / 2.0);

    // m_volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    // m_volumeProperty->SetInterpolationTypeToLinear();
    // m_volumeProperty->ShadeOn();
    // m_volumeProperty->SetAmbient(0.4);
    // m_volumeProperty->SetDiffuse(0.6);
    // m_volumeProperty->SetSpecular(0.2);
    // m_volumeProperty->SetColor(ctf3d);
    // m_volumeProperty->SetScalarOpacity(pwf3d);

    // vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    // volume->SetMapper(volumeMapper);
    // volume->SetProperty(m_volumeProperty);

    // Depends on the machine; the developer laptop has no GPU and shows artifacts. Consider vtkSmartVolumeMapper.
    vtkSmartPointer<vtkFixedPointVolumeRayCastMapper> volumeMapper = vtkSmartPointer<vtkFixedPointVolumeRayCastMapper>::New();
    volumeMapper->SetInputData(m_imageData);

    volumeMapper->SetAutoAdjustSampleDistances(0);
    volumeMapper->SetIntermixIntersectingGeometry(1);

    double avgSpacing = (spacing[0] + spacing[1] + spacing[2]) / 3.0;
    
    volumeMapper->SetSampleDistance(avgSpacing * 1.0); 
    volumeMapper->SetImageSampleDistance(1.0); 

    m_volumeProperty = vtkSmartPointer<vtkVolumeProperty>::New();
    m_volumeProperty->SetInterpolationTypeToLinear();
    m_volumeProperty->ShadeOn();
    m_volumeProperty->SetAmbient(0.4);
    m_volumeProperty->SetDiffuse(0.6);
    m_volumeProperty->SetSpecular(0.2);
    m_volumeProperty->SetColor(ctf3d);
    m_volumeProperty->SetScalarOpacity(pwf3d);

    vtkSmartPointer<vtkVolume> volume = vtkSmartPointer<vtkVolume>::New();
    volume->SetMapper(volumeMapper);
    volume->SetProperty(m_volumeProperty);

    if (m_actors.size() >= 1)
    {
        m_actors.insert(m_actors.begin() + 1, volume);
    }
}

void Image::adjustActors(double current_center[3])
{
    for (size_t i = 0; i < m_reslices.size(); ++i)
    {
        if (m_reslices[i])
        {
            vtkMatrix4x4* matrix = m_reslices[i]->GetResliceAxes();
            if (matrix)
            {
                vtkSmartPointer<vtkMatrix4x4> newMatrix = vtkSmartPointer<vtkMatrix4x4>::New();
                newMatrix->DeepCopy(matrix);
                
                int axisIndex = (2 - i) % 3;
                double origin[3] = {0.0, 0.0, 0.0};
                origin[0] = matrix->GetElement(0, 3);
                origin[1] = matrix->GetElement(1, 3);
                origin[2] = matrix->GetElement(2, 3);
                
                origin[axisIndex] = current_center[axisIndex];

                newMatrix->SetElement(0, 3, origin[0]);
                newMatrix->SetElement(1, 3, origin[1]);
                newMatrix->SetElement(2, 3, origin[2]);

                m_reslices[i]->SetResliceAxes(newMatrix);
            }
        }
    }
}

void Image::getOrthogonalMatrix(int direction, vtkMatrix4x4* matrix)
{
    matrix->Identity();
    if (direction == 0) // Axial (viewed from inferior to superior).
    {
        matrix->SetElement(0, 0, 1);  matrix->SetElement(0, 1, 0);  matrix->SetElement(0, 2, 0);
        matrix->SetElement(1, 0, 0);  matrix->SetElement(1, 1, -1); matrix->SetElement(1, 2, 0);
        matrix->SetElement(2, 0, 0);  matrix->SetElement(2, 1, 0);  matrix->SetElement(2, 2, -1);
    }
    else if (direction == 1) // Coronal (viewed from the front).
    {
        matrix->SetElement(0, 0, -1);  matrix->SetElement(0, 1, 0);  matrix->SetElement(0, 2, 0);
        matrix->SetElement(1, 0, 0);  matrix->SetElement(1, 1, 0);  matrix->SetElement(1, 2, 1);
        matrix->SetElement(2, 0, 0);  matrix->SetElement(2, 1, 1);  matrix->SetElement(2, 2, 0);
    }
    else if (direction == 2) // Sagittal (viewed from the side).
    {
        matrix->SetElement(0, 0, 0);  matrix->SetElement(0, 1, 0);  matrix->SetElement(0, 2, 1);
        matrix->SetElement(1, 0, -1);  matrix->SetElement(1, 1, 0);  matrix->SetElement(1, 2, 0);
        matrix->SetElement(2, 0, 0);  matrix->SetElement(2, 1, 1);  matrix->SetElement(2, 2, 0);
    }
}
