/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#include "itkDCMTKTransformIO.h"
#include "itkDCMTKTransformIOFactory.h"
#include "itkTransformFileReader.h"
#include "itkImageSeriesReader.h"
#include "itkDCMTKImageIO.h"
#include "itkDCMTKSeriesFileNames.h"
#include "itkGDCMImageIO.h"
#include "itkGDCMSeriesFileNames.h"
#include "itkCompositeTransform.h"
#include "itkImageFileWriter.h"
#include "itkMetaDataObject.h"
#include "itkResampleImageFilter.h"


int ReadDicomTransformAndResampleExample( int argc, char* argv[] )
{
  // Parse command line arguments
  if( argc < 5 )
    {
    std::cerr << "Usage: " << argv[0]
              << " fixedSeriesDirectory movingSeriesDirectory transform fixedImageOutput resampledMovingOutput" << std::endl;
    return EXIT_FAILURE;
    }
  const char * fixedSeriesDirectory = argv[1];
  const char * movingSeriesDirectory = argv[2];
  const char * transformFileName = argv[3];
  const char * fixedImageOutputFileName = argv[4];
  const char * resampledMovingOutputFileName = argv[5];


  // Basic types
  const unsigned int Dimension = 3;
  typedef short                              PixelType;
  typedef itk::Image< PixelType, Dimension > ImageType;


  // Read the fixed and moving image
  typedef itk::ImageSeriesReader< ImageType > ReaderType;
  ReaderType::Pointer fixedReader = ReaderType::New();

  // DCMTKImageIO does not populate the MetaDataDictionary yet
  //typedef itk::DCMTKImageIO ImageIOType;
  typedef itk::GDCMImageIO ImageIOType;
  ImageIOType::Pointer fixedIO = ImageIOType::New();
  fixedReader->SetImageIO( fixedIO );

  //typedef itk::DCMTKSeriesFileNames SeriesFileNamesType;
  typedef itk::GDCMSeriesFileNames SeriesFileNamesType;
  SeriesFileNamesType::Pointer fixedSeriesFileNames = SeriesFileNamesType::New();
  fixedSeriesFileNames->SetInputDirectory( fixedSeriesDirectory );
  typedef SeriesFileNamesType::FileNamesContainerType FileNamesContainerType;
  const FileNamesContainerType & fixedFileNames = fixedSeriesFileNames->GetInputFileNames();
  std::cout << "There are " << fixedFileNames.size() << " fixed image slices." << std::endl;
  std::cout << "First fixed images series UID: " << fixedSeriesFileNames->GetSeriesUIDs()[0] << "\n" << std::endl;
  fixedReader->SetFileNames( fixedFileNames );

  ReaderType::Pointer movingReader = ReaderType::New();
  ImageIOType::Pointer movingIO = ImageIOType::New();
  movingReader->SetImageIO( movingIO );

  SeriesFileNamesType::Pointer movingSeriesFileNames = SeriesFileNamesType::New();
  movingSeriesFileNames->SetInputDirectory( movingSeriesDirectory );
  const FileNamesContainerType &  movingFileNames = movingSeriesFileNames->GetInputFileNames();
  std::cout << "There are " << movingFileNames.size() << " moving image slices." << std::endl;
  std::cout << "First moving images series UID: " << movingSeriesFileNames->GetSeriesUIDs()[0] << "\n" << std::endl;
  movingReader->SetFileNames( movingFileNames );

  try
    {
    fixedReader->Update();
    movingReader->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }


  // Create a DICOM transform reader
  typedef float ScalarType;

  itk::DCMTKTransformIOFactory::Pointer dcmtkTransformIOFactory = itk::DCMTKTransformIOFactory::New();
  itk::ObjectFactoryBase::RegisterFactory( dcmtkTransformIOFactory );

  typedef itk::TransformFileReaderTemplate< ScalarType > TransformReaderType;
  TransformReaderType::Pointer transformReader = TransformReaderType::New();
  transformReader->SetFileName( transformFileName );

  typedef itk::DCMTKTransformIO< ScalarType > TransformIOType;
  TransformIOType::Pointer transformIO = TransformIOType::New();
  transformReader->SetTransformIO( transformIO );


  // Read in the fixed image transform
  const ReaderType::DictionaryType & fixedMetaDataDict = fixedIO->GetMetaDataDictionary();
  std::string fixedFrameOfReferenceUID;
  if( ! itk::ExposeMetaData< std::string >( fixedMetaDataDict, "0020|0052", fixedFrameOfReferenceUID ) )
    {
    std::cerr << "Could not find the fixed image frame of reference UID." << std::endl;
    return EXIT_FAILURE;
    }
  std::cout << "Fixed image frame of reference UID: " << fixedFrameOfReferenceUID << std::endl;
  transformIO->SetFrameOfReferenceUID( fixedFrameOfReferenceUID );

  try
    {
    transformReader->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }
  typedef TransformReaderType::TransformListType TransformListType;
  TransformListType * transformList = transformReader->GetTransformList();

  typedef itk::CompositeTransform< ScalarType, Dimension > ReadTransformType;
  TransformListType::const_iterator transformIt = transformList->begin();
  ReadTransformType::Pointer fixedTransform = dynamic_cast< ReadTransformType * >( (*transformIt).GetPointer() );
  if( fixedTransform.IsNull() )
    {
    std::cerr << "Did not get the expected transform out." << std::endl;
    return EXIT_FAILURE;
    }
  std::cout << "Fixed transform: " << fixedTransform << std::endl;


  // Read in the moving image transform
  const ReaderType::DictionaryType & movingMetaDataDict = movingIO->GetMetaDataDictionary();
  std::string movingFrameOfReferenceUID;
  if( ! itk::ExposeMetaData< std::string >( movingMetaDataDict, "0020|0052", movingFrameOfReferenceUID ) )
    {
    std::cerr << "Could not find the moving image frame of reference UID." << std::endl;
    return EXIT_FAILURE;
    }
  std::cout << "Moving image frame of reference UID: " << movingFrameOfReferenceUID << std::endl;
  transformIO->SetFrameOfReferenceUID( movingFrameOfReferenceUID );

  try
    {
    transformReader->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }

  transformList = transformReader->GetTransformList();
  transformIt = transformList->begin();
  ReadTransformType::Pointer movingTransform = dynamic_cast< ReadTransformType * >( (*transformIt).GetPointer() );
  if( movingTransform.IsNull() )
    {
    std::cerr << "Did not get the expected transform out." << std::endl;
    return EXIT_FAILURE;
    }
  std::cout << "Moving transform: " << movingTransform << std::endl;


  // Compose the transform from the fixed to the moving image
  ReadTransformType::Pointer movingTransformInverse = ReadTransformType::New();
  movingTransform->GetInverse( movingTransformInverse );

  ReadTransformType::Pointer fixedToMovingTransform = ReadTransformType::New();
  fixedToMovingTransform->AddTransform( fixedTransform );
  fixedToMovingTransform->AddTransform( movingTransformInverse );
  // Flatten out the two component CompositeTransforms.
  fixedToMovingTransform->FlattenTransformQueue();

  typedef itk::ResampleImageFilter< ImageType, ImageType, ScalarType, ScalarType > ResamplerType;
  ResamplerType::Pointer resampler = ResamplerType::New();
  resampler->SetInput( movingReader->GetOutput() );
  resampler->SetUseReferenceImage( true );
  resampler->SetReferenceImage( fixedReader->GetOutput() );
  resampler->SetTransform( fixedToMovingTransform );
  resampler->SetDefaultPixelValue( -1000 );


  // Write the fixed image and resampled moving image (should look similar)
  typedef itk::ImageFileWriter< ImageType > WriterType;
  WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( fixedImageOutputFileName );
  writer->SetInput( fixedReader->GetOutput() );
  try
    {
    writer->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }

  writer->SetInput( resampler->GetOutput() );
  writer->SetFileName( resampledMovingOutputFileName );
  try
    {
    writer->Update();
    }
  catch( itk::ExceptionObject & error )
    {
    std::cerr << "Error: " << error << std::endl;
    return EXIT_FAILURE;
    }


  return EXIT_SUCCESS;
}
