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
#ifndef itkAdjointOrientedGaussianInterpolateImageFilter_hxx
#define itkAdjointOrientedGaussianInterpolateImageFilter_hxx

#include "itkAdjointOrientedGaussianInterpolateImageFilter.h"
#include "itkObjectFactory.h"
#include "itkIdentityTransform.h"
#include "itkProgressReporter.h"
#include "itkImageRegionIteratorWithIndex.h"
#include "itkImageScanlineIterator.h"
#include "itkSpecialCoordinatesImage.h"
#include "itkDefaultConvertPixelTraits.h"

namespace itk
{
/**
 * Initialize new instance
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::AdjointOrientedGaussianInterpolateImageFilter()
{
  this->m_OutputOrigin.Fill(0.0);
  this->m_OutputSpacing.Fill(1.0);
  this->m_OutputDirection.SetIdentity();

  this->m_UseReferenceImage = false;

  this->m_Size.Fill(0);
  this->m_OutputStartIndex.Fill(0);

  // Pipeline input configuration

  // implicit:
  // #0 "Primary" required

  //  #1 "ReferenceImage" optional
  Self::AddRequiredInputName("ReferenceImage",1);
  Self::RemoveRequiredInputName("ReferenceImage");

  //   "Transform" required ( not numbered )
  Self::AddRequiredInputName("Transform");
  Self::SetTransform(IdentityTransform< TTransformPrecisionType, ImageDimension >::New());

  this->m_Interpolator = dynamic_cast< InterpolatorType * >
    ( LinearInterpolatorType::New().GetPointer() );

  this->m_Extrapolator = ITK_NULLPTR;

  this->m_DefaultPixelValue
    = NumericTraits<PixelType>::ZeroValue( m_DefaultPixelValue );

  this->m_Alpha = 1.0;
  this->m_Sigma.Fill( 1.0 );
  this->m_Covariance.Fill( 0.0 );

  for( unsigned int d = 0; d < ImageDimension; d++ )
    {
    this->m_Covariance[d*ImageDimension + d] = 1.0;
    }
}

/**
 * Print out a description of self
 *
 * \todo Add details about this class
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);

  os << indent << "DefaultPixelValue: "
     << static_cast< typename NumericTraits< PixelType >::PrintType >
  ( m_DefaultPixelValue )
     << std::endl;
  os << indent << "Size: " << m_Size << std::endl;
  os << indent << "OutputStartIndex: " << m_OutputStartIndex << std::endl;
  os << indent << "OutputSpacing: " << m_OutputSpacing << std::endl;
  os << indent << "OutputOrigin: " << m_OutputOrigin << std::endl;
  os << indent << "OutputDirection: " << m_OutputDirection << std::endl;
  os << indent << "Transform: " << this->GetTransform() << std::endl;
  os << indent << "Interpolator: " << m_Interpolator.GetPointer() << std::endl;
  os << indent << "Extrapolator: " << m_Extrapolator.GetPointer() << std::endl;
  os << indent << "UseReferenceImage: " << ( m_UseReferenceImage ? "On" : "Off" )
     << std::endl;
}

template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::ComputeBoundingBox()
{
  if( !this->GetOutput() )
    {
    return;
    }

  typename OutputImageType::ConstPointer output = this->GetOutput();
  typename OutputImageType::SpacingType spacing = output->GetSpacing();
  typename OutputImageRegionType::SizeType size = output->GetBufferedRegion().GetSize();

  // std::cout << "spacing = " << spacing << std::endl;
  // std::cout << "size = " << size << std::endl;

  for( unsigned int d = 0; d < ImageDimension; d++ )
    {
    this->m_BoundingBoxStart[d] = -0.5;
    this->m_BoundingBoxEnd[d] = static_cast<RealType>( size[d] ) - 0.5;
    this->m_CutoffDistance[d] = this->m_Sigma[d] * this->m_Alpha / spacing[d];
    }
}

/**
 * Set the output image spacing.
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::SetOutputSpacing(const double *spacing)
{
  SpacingType s;
  for(unsigned int i = 0; i < TOutputImage::ImageDimension; ++i)
    {
    s[i] = static_cast< typename SpacingType::ValueType >(spacing[i]);
    }
  this->SetOutputSpacing(s);
}

/**
 * Set the output image origin.
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::SetOutputOrigin(const double *origin)
{
  OriginPointType p(origin);

  this->SetOutputOrigin(p);
}

/** Helper method to set the output parameters based on this image */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::SetOutputParametersFromImage(const ImageBaseType *image)
{
  this->SetOutputOrigin ( image->GetOrigin() );
  this->SetOutputSpacing ( image->GetSpacing() );
  this->SetOutputDirection ( image->GetDirection() );
  this->SetOutputStartIndex ( image->GetLargestPossibleRegion().GetIndex() );
  this->SetSize ( image->GetLargestPossibleRegion().GetSize() );
}

/**
 * Cast from interpolotor output to pixel type
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
typename AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::PixelType
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::CastPixelWithBoundsChecking(const InterpolatorOutputType value,
                              const ComponentType minComponent,
                              const ComponentType maxComponent ) const
{
  const unsigned int nComponents = InterpolatorConvertType::GetNumberOfComponents(value);
  PixelType          outputValue;

  NumericTraits<PixelType>::SetLength( outputValue, nComponents );

  for (unsigned int n=0; n<nComponents; n++)
    {
    ComponentType component = InterpolatorConvertType::GetNthComponent( n, value );

    if ( component < minComponent )
      {
      PixelConvertType::SetNthComponent( n, outputValue, static_cast<PixelComponentType>( minComponent ) );
      }
    else if ( component > maxComponent )
      {
      PixelConvertType::SetNthComponent( n, outputValue, static_cast<PixelComponentType>( maxComponent ) );
      }
    else
      {
      PixelConvertType::SetNthComponent(n, outputValue,
                                        static_cast<PixelComponentType>( component ) );
      }
    }

  return outputValue;
}


/**
 * GenerateData
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::GenerateData()
{
  // Call a method that can be overriden by a subclass to allocate
  // memory for the filter's outputs
  this->AllocateOutputs();

  // Call a method that can be overridden by a subclass to perform
  // some calculations prior to splitting the main computations into
  // separate threads
  this->BeforeThreadedGenerateData();

  // Set up the multithreaded processing
  ThreadStruct str;
  str.Filter = this;

  // Get the input pointer
  typename InputImageType::ConstPointer inputPtr = this->GetInput();

  const ImageRegionSplitterBase * splitter = this->GetImageRegionSplitter();
  const unsigned int validThreads = splitter->GetNumberOfSplits( inputPtr->GetRequestedRegion(), this->GetNumberOfThreads() );

  this->GetMultiThreader()->SetNumberOfThreads( validThreads );
  this->GetMultiThreader()->SetSingleMethod(this->ThreaderCallback, &str);

  // multithread the execution
  this->GetMultiThreader()->SingleMethodExecute();

  std::cout << this->GetMultiThreader() << std::endl;

  // Call a method that can be overridden by a subclass to perform
  // some calculations after all the threads have completed
  this->AfterThreadedGenerateData();

}

/**
 * Set up state of filter before multi-threading.
 * InterpolatorType::SetInputImage is not thread-safe and hence
 * has to be set up before ThreadedGenerateData
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::BeforeThreadedGenerateData()
{
  // Get the output pointer
  typename OutputImageType::Pointer outputPtr = this->GetOutput();

  // Initialize output
  outputPtr->FillBuffer( itk::NumericTraits< PixelType >::Zero );

  // Compute bounding box for Gaussian exponential
  this->ComputeBoundingBox();


  // const itk::ThreadIdType numberOfThreads = this->GetNumberOfThreads();
  // // std::cout << "numberOfThreads = " << numberOfThreads << std::endl;

  // this->m_OutputPtrThread.resize( numberOfThreads );
  // // m_OutputPtrThread = new OutputImageType[numberOfThreads];


  // for( itk::ThreadIdType ii = 0; ii < numberOfThreads; ++ii )
  //   {
  //     // m_OutputPtrThread[ii] = std::make_shared<OutputImageType>(dynamic_cast<OutputImageType>(outputPtr));
  //     // m_OutputPtrThread[ii] = std::make_shared<OutputImageType>(OutputImageType::New());
  //     // m_OutputPtrThread[ii] = std::shared_ptr<OutputImageType>();
  //     // m_OutputPtrThread[ii] = dynamic_cast<OutputImagePointer>(outputPtr);
  //     m_OutputPtrThread[ii] = OutputImageType::New();
  //     // std::cout << m_OutputPtrThread[ii].GetSpacing() << std::endl;
  //   }


}

// Callback routine used by the threading library. This routine just calls
// the ThreadedGenerateData method after setting the correct region for this
// thread.
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
ITK_THREAD_RETURN_TYPE
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::ThreaderCallback(void *arg)
{
  ThreadStruct *str;
  ThreadIdType  total, threadId, threadCount;

  threadId = ( (MultiThreader::ThreadInfoStruct *)( arg ) )->ThreadID;
  threadCount = ( (MultiThreader::ThreadInfoStruct *)( arg ) )->NumberOfThreads;

  str = (ThreadStruct *)( ( (MultiThreader::ThreadInfoStruct *)( arg ) )->UserData );

  // execute the actual method with appropriate input region
  // first find out how many pieces extent can be split into.
  typename TInputImage::RegionType splitRegion;
  total = str->Filter->SplitRequestedRegion(threadId, threadCount,
                                            splitRegion);

  if ( threadId < total )
    {
    str->Filter->ThreadedGenerateData(splitRegion, threadId);
    }
  // else
  //   {
  //   otherwise don't use this thread. Sometimes the threads dont
  //   break up very well and it is just as efficient to leave a
  //   few threads idle.
  //   }

  return ITK_THREAD_RETURN_VALUE;
}


/**
 * Set up state of filter after multi-threading.
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::AfterThreadedGenerateData()
{

}


/**
 * ThreadedGenerateData
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::ThreadedGenerateData(const InputImageRegionType & inputRegionForThread,
                       ThreadIdType threadId)
{

  // Get the output pointer
  typename OutputImageType::Pointer outputPtr = this->GetOutput();

  // Get the input pointer
  typename InputImageType::ConstPointer inputPtr = this->GetInput();


  // Get the input transform
  // const TransformType *transformPtr = this->GetTransform();
  typename TransformType::ConstPointer transformPtr = this->GetTransform();

  // Get input and output region
  // InputImageRegionType inputRegion = inputPtr->GetBufferedRegion();

  // Create an iterator that will walk the input region for this thread.
  typedef ImageRegionConstIteratorWithIndex< TInputImage > InputIterator;
  InputIterator inIt(inputPtr, inputRegionForThread);

  // Get input and output region size
  // const typename InputImageRegionType::SizeType &inputRegionSize = inputRegion.GetSize();
  // const typename OutputImageRegionType::SizeType &outputRegionSize = outputPtr->GetBufferedRegion().GetSize();

  // Define a few indices that will be used to translate from an input pixel
  // to an output pixel
  PointType outputPoint;         // Coordinates of current output pixel
  PointType inputPoint;          // Coordinates of current input pixel

  ContinuousInputIndexType inputCIndex;
  ContinuousOutputIndexType outputCIndex;

  IndexType inputIndex;
  IndexType outputIndex;

  typedef typename InterpolatorType::OutputType OutputType;
  PixelType pixval;
  OutputType value;

  // Support for progress methods/callbacks
  ProgressReporter progress( this,
                             threadId,
                             inputRegionForThread.GetNumberOfPixels() );


  // Min/max values of the output pixel type AND these values
  // represented as the output type of the interpolator
  const PixelComponentType minValue =  NumericTraits< PixelComponentType >::NonpositiveMin();
  const PixelComponentType maxValue =  NumericTraits< PixelComponentType >::max();
  const ComponentType minOutputValue = static_cast< ComponentType >( minValue );
  const ComponentType maxOutputValue = static_cast< ComponentType >( maxValue );

  // Cache information from the superclass
  // PixelType defaultValue = this->GetDefaultPixelValue();

  // Create an iterator that will walk the input region
  // ImageRegionConstIteratorWithIndex<InputImageType> inIt( inputPtr, inputRegion );


  // ME: Compute scaled oriented PSF for voxel space
  // Scaling matrix
  const typename OutputImageType::SpacingType spacing = outputPtr->GetSpacing();
  itk::Matrix<double,ImageDimension,ImageDimension> S;
  S.Fill(0.0);
  for (int d = 0; d < ImageDimension; ++d)
    {
    S(d,d) = spacing[d];
    }
  // std::cout << "S = \n" << S << std::endl;

  // Scale rotated inverse Gaussian needed for exponential function
  itk::Matrix<double, ImageDimension, ImageDimension> covariance;
  for (int i = 0; i < ImageDimension; i++ )
  {
    for (int j = 0; j < ImageDimension; j++ )
    {
      covariance(i,j) = this->m_Covariance[i*ImageDimension + j];
    }
  }

  itk::Matrix<double, ImageDimension, ImageDimension> CovScaledInv = S * covariance.GetInverse() * S;
  // std::cout << "CovScaledInv = \n" << CovScaledInv << std::endl;

  // std::cout << "m_CutoffDistance = " << m_CutoffDistance << std::endl;
  // std::cout << "m_BoundingBoxStart = " << m_BoundingBoxStart << std::endl;
  // std::cout << "m_BoundingBoxEnd = " << m_BoundingBoxEnd << std::endl;

  // Walk the input region
  inIt.GoToBegin();

  while ( !inIt.IsAtEnd() )
    {

      // Determine the position of the first pixel in the scanline
      inputIndex = inIt.GetIndex();
      inputPtr->TransformIndexToPhysicalPoint(inputIndex, inputPoint);

      // Compute corresponding output pixel position
      outputPoint = transformPtr->TransformPoint(inputPoint);
      outputPtr->TransformPhysicalPointToContinuousIndex(outputPoint, outputCIndex);

      // if ( outputRegion.IsInside(outputIndex) )
      // {
      //   pixval = inputPtr->GetPixel(inputIndex);
      //   outputPtr->SetPixel(outputIndex,pixval);
      // }

      // Loop over the voxels in the region identified
      OutputImageRegionType outputRegion;
      for( unsigned int d = 0; d < ImageDimension; d++ )
        {
        int boundingBoxSize = static_cast<int>(
          this->m_BoundingBoxEnd[d] - this->m_BoundingBoxStart[d] + 0.5 );  // = size[d]
        int begin = vnl_math_max( 0, static_cast<int>( std::floor( outputCIndex[d] -
          this->m_BoundingBoxStart[d] - this->m_CutoffDistance[d] ) ) );
        int end = vnl_math_min( boundingBoxSize, static_cast<int>( std::ceil(
          outputCIndex[d] - this->m_BoundingBoxStart[d] + this->m_CutoffDistance[d] ) ) );

        begin = vnl_math_min(begin,boundingBoxSize);  //to prevent begin>boundingBoxSize
        end = vnl_math_max(end,0);                    //to prevent end<0

        outputRegion.SetIndex( d, begin );
        outputRegion.SetSize( d, end - begin );

        // std::cout << "begin = " << begin << ", end = " << end << std::endl;
        // std::cout << "boundingBoxSize = " << boundingBoxSize << std::endl;
        }

      // std::cout << "outputRegion = " << outputRegion << std::endl;

      // ME: Define iterator over chosen region
      ImageRegionIteratorWithIndex<OutputImageType> outIt( outputPtr, outputRegion );

      RealType w = 0.0;
      RealType sum_m = 0.0;


      for( outIt.GoToBegin(); !outIt.IsAtEnd(); ++outIt )
        {
          w = this->ComputeExponentialFunction(outIt.GetIndex(), outputCIndex, CovScaledInv);
          sum_m += w;
        }

      for( outIt.GoToBegin(); !outIt.IsAtEnd(); ++outIt )
        {
          w = this->ComputeExponentialFunction(outIt.GetIndex(), outputCIndex, CovScaledInv);
          value = outIt.Get() + inIt.Get()*w/sum_m;
          // pixval = this->CastPixelWithBoundsChecking( value, minOutputValue, maxOutputValue );
          // std::cout << "weight = " << w << std::endl;
          // std::cout << "weight_sum = " << sum_m << std::endl;
          // std::cout << "inIt.Get() = " << inIt.Get() << std::endl;
          // std::cout << "value = Get() + inIt.Get()*w/sum_m  = " << value << std::endl;
          // std::cout << "value = outIt.Get() + inIt.Get()*w/sum_m  = " << outIt.Get() << " + " << inIt.Get() << "*" << w << "/" << sum_m << " = " << value << std::endl;
          outIt.Set(value);
        }

      ++inIt;
    }

  // // DEBUGGING OUTPUT
  // std::cout << "sizeInput = " << inputRegionSize << std::endl;
  // std::cout << "sizeOutput = " << outputRegionSize << std::endl;
  // std::cout << "inputIndex = " << inputIndex << std::endl;
  // std::cout << "inputPoint = " << inputPoint << std::endl;
  // std::cout << "outputPoint = " << outputPoint << std::endl;
  // std::cout << "outputIndex = " << outputIndex << std::endl;
  // std::cout << "outputCIndex = " << outputCIndex << std::endl;

  // ImageAlgorithm::Copy(inputPtr, outputPtr, outputPtr->GetRequestedRegion(),
  //                      outputPtr->GetRequestedRegion() );

}


template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
typename AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::RealType
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::ComputeExponentialFunction(
  IndexType point,
  ContinuousOutputIndexType center,
  itk::Matrix<double, ImageDimension, ImageDimension> CovScaledInv ) const
{
  itk::Vector<double, ImageDimension> tmp;
  itk::Vector<double, ImageDimension> diff;
  RealType result;

  for (int i = 0; i < ImageDimension; ++i)
  {
    diff[i] = point[i] - center[i];
    // printf("diff[%d] = %f\n", i, diff[i]);
  }


  tmp = CovScaledInv*diff;
  result = diff*tmp;

  // std::cout << "Sigma*(point-center) = " << tmp << std::endl;
  // std::cout << "(point-center)'*Sigma*(point-center) = " << result << std::endl;

  result = exp( -0.5*result );
  // std::cout << "exp(.) = " << result << std::endl;

  return result;
}

/**
 * Inform pipeline of necessary input image region
 *
 * Determining the actual input region is non-trivial, especially
 * when we cannot assume anything about the transform being used.
 * So we do the easy thing and request the entire input image.
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::GenerateInputRequestedRegion()
{
  // call the superclass's implementation of this method
  Superclass::GenerateInputRequestedRegion();

  if ( !this->GetInput() )
    {
    return;
    }

  // get pointers to the input and output
  InputImagePointer inputPtr  =
    const_cast< TInputImage * >( this->GetInput() );

  // Request the entire input image
  inputPtr->SetRequestedRegionToLargestPossibleRegion();
}

/**
 * Inform pipeline of required output region
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
void
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::GenerateOutputInformation()
{
  // call the superclass' implementation of this method
  Superclass::GenerateOutputInformation();

  // get pointers to the input and output
  OutputImageType *outputPtr = this->GetOutput();
  if ( !outputPtr )
    {
    return;
    }

  const ReferenceImageBaseType *referenceImage = this->GetReferenceImage();

  // Set the size of the output region
  if ( m_UseReferenceImage && referenceImage )
    {
    outputPtr->SetLargestPossibleRegion(
      referenceImage->GetLargestPossibleRegion() );
    }
  else
    {
    typename TOutputImage::RegionType outputLargestPossibleRegion;
    outputLargestPossibleRegion.SetSize(m_Size);
    outputLargestPossibleRegion.SetIndex(m_OutputStartIndex);
    outputPtr->SetLargestPossibleRegion(outputLargestPossibleRegion);
    }

  // Set spacing and origin
  if ( m_UseReferenceImage && referenceImage )
    {
    outputPtr->SetSpacing( referenceImage->GetSpacing() );
    outputPtr->SetOrigin( referenceImage->GetOrigin() );
    outputPtr->SetDirection( referenceImage->GetDirection() );
    }
  else
    {
    outputPtr->SetSpacing(m_OutputSpacing);
    outputPtr->SetOrigin(m_OutputOrigin);
    outputPtr->SetDirection(m_OutputDirection);
    }
}

/**
 * Verify if any of the components has been modified.
 */
template< typename TInputImage,
          typename TOutputImage,
          typename TInterpolatorPrecisionType,
          typename TTransformPrecisionType >
ModifiedTimeType
AdjointOrientedGaussianInterpolateImageFilter< TInputImage, TOutputImage, TInterpolatorPrecisionType, TTransformPrecisionType >
::GetMTime(void) const
{
  ModifiedTimeType latestTime = Object::GetMTime();

  if ( m_Interpolator )
    {
    if ( latestTime < m_Interpolator->GetMTime() )
      {
      latestTime = m_Interpolator->GetMTime();
      }
    }

  return latestTime;
}

} // end namespace itk

#endif
