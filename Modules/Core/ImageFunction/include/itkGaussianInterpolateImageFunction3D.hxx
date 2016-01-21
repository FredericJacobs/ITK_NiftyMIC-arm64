/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkGaussianInterpolateImageFunction.hxx,v $
  Language:  C++
  Date:      $Date: $
  Version:   $Revision: $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef itkGaussianInterpolateImageFunction3D_hxx
#define itkGaussianInterpolateImageFunction3D_hxx

#include "itkGaussianInterpolateImageFunction3D.h"

#include "itkImageRegionConstIteratorWithIndex.h"

namespace itk
{

/**
 * Constructor
 */
template<typename TImageType, typename TCoordRep>
GaussianInterpolateImageFunction<TImageType, TCoordRep>
::GaussianInterpolateImageFunction()
{
  this->m_Alpha = 1.0;       //TODO: set alpha-vector
  this->m_Sigma.Fill( 1.0 ); //TODO: set identity-matrix
}

/**
 * Standard "PrintSelf" method
 */
template<typename TImageType, typename TCoordRep>
void
GaussianInterpolateImageFunction<TImageType, TCoordRep>
::PrintSelf( std::ostream& os, Indent indent ) const
{
  Superclass::PrintSelf( os, indent );
  os << indent << "Alpha: " << this->m_Alpha << std::endl;
  os << indent << "Sigma: " << this->m_Sigma << std::endl;
}

template<typename TImageType, typename TCoordRep>
void
GaussianInterpolateImageFunction<TImageType, TCoordRep>
::ComputeBoundingBox()
{
  if( !this->GetInputImage() )
    {
    return;
    }

  typename InputImageType::ConstPointer input = this->GetInputImage();
  typename InputImageType::SpacingType spacing = input->GetSpacing();
  typename InputImageType::SizeType size = input->GetBufferedRegion().GetSize();

  for( unsigned int d = 0; d < ImageDimension; d++ )
    {
    this->m_BoundingBoxStart[d] = -0.5;
    // ? ME: static_cast<RealType>( size[d] )?
    this->m_BoundingBoxEnd[d] = static_cast<RealType>( size[d] ) - 0.5;
    this->m_ScalingFactor[d] = 1.0 / ( vnl_math::sqrt2 * this->m_Sigma[d] / spacing[d] ); //TODO: Only dependent on spacing
    this->m_CutoffDistance[d] = this->m_Sigma[d] * this->m_Alpha / spacing[d]; //TODO: alpha in all directions
    }
}


// ME: Compute intensity value of one single voxel based on Gaussian interpolation
template<typename TImageType, typename TCoordRep>
typename GaussianInterpolateImageFunction<TImageType, TCoordRep>
::OutputType
GaussianInterpolateImageFunction<TImageType, TCoordRep>
::EvaluateAtContinuousIndex( const ContinuousIndexType & cindex, OutputType *grad ) const
{
  // ME: Where is ImageDimension defined?
  vnl_vector<RealType> erfArray[ImageDimension];  // ME: error function
  vnl_vector<RealType> gerfArray[ImageDimension]; // ME: gradient error function

  // Compute the ERF difference arrays
  for( unsigned int d = 0; d < ImageDimension; d++ )
    {
    bool evaluateGradient = false;
    if( grad )
      {
      evaluateGradient = true;
      }
    this->ComputeErrorFunctionArray( d, cindex[d], erfArray[d],
      gerfArray[d], evaluateGradient );
    }

  RealType sum_me = 0.0;
  RealType sum_m = 0.0;
  ArrayType dsum_me;
  ArrayType dsum_m;
  ArrayType dw;

  dsum_m.Fill( 0.0 );
  dsum_me.Fill( 0.0 );
  dw.Fill( 0.0 );

  // Loop over the voxels in the region identified
  // ME: First, compute region which is to be considered, i.e. have non-zero Gaussian weights
  // ? ME: Is this a rectangular bounding box?
  ImageRegion<ImageDimension> region;
  for( unsigned int d = 0; d < ImageDimension; d++ )
    {
    int boundingBoxSize = static_cast<int>(
      this->m_BoundingBoxEnd[d] - this->m_BoundingBoxStart[d] + 0.5 );  // = size[d]
    int begin = vnl_math_max( 0, static_cast<int>( std::floor( cindex[d] -
      this->m_BoundingBoxStart[d] - this->m_CutoffDistance[d] ) ) );
    int end = vnl_math_min( boundingBoxSize, static_cast<int>( std::ceil(
      cindex[d] - this->m_BoundingBoxStart[d] + this->m_CutoffDistance[d] ) ) );
    region.SetIndex( d, begin );
    region.SetSize( d, end - begin );
    }

  // ME: Define iterator over chosen region
  ImageRegionConstIteratorWithIndex<InputImageType> It(
    this->GetInputImage(), region );

  // ME: For each voxel of that region do
  for( It.GoToBegin(); !It.IsAtEnd(); ++It )
    {
    unsigned int j = It.GetIndex()[0];    // ? ME: What do I get here exactly?
    RealType w = erfArray[0][j];          // ME: Get weight of current element (first coordinate out of ImageDimension dimensions)
    if( grad )
      {
      dw[0] = gerfArray[0][j];
      for( unsigned int d = 1; d < ImageDimension; d++ )
        {
        dw[d] = erfArray[0][j];
        }
      }
    // ME: Compute weight of voxel
    for( unsigned int d = 1; d < ImageDimension; d++)
      {
      j = It.GetIndex()[d];
      // ? ME: This must assume a diagonal covariance matrix!?
      // ? ME: Otherwise not just a simple multiplication of weights
      w *= erfArray[d][j];    // ME: Update weight
      if( grad )
        {
        for( unsigned int q = 0; q < ImageDimension; q++ )
          {
          if( d == q )
            {
            dw[q] *= gerfArray[d][j];
            }
          else
            {
            dw[q] *= erfArray[d][j];
            }
          }
        }
      }
    RealType V = It.Get();  // ME: Intensity of current voxel
    sum_me += V * w;        // ME: Add Gaussian weighted intensity
    sum_m += w;             // ME: Record weight sum for subsequent normalization
    if( grad )
      {
      for( unsigned int q = 0; q < ImageDimension; q++ )
        {
        dsum_me[q] += V * dw[q];
        dsum_m[q] += dw[q];
        }
      }
    }
  RealType rc = sum_me / sum_m;   // ME: Final Gaussian interpolated voxel intensity

  if( grad )
    {
    for( unsigned int q = 0; q < ImageDimension; q++ )
      {
      grad[q] = ( dsum_me[q] - rc * dsum_m[q] ) / sum_m;
      grad[q] /= -vnl_math::sqrt2 * this->m_Sigma[q];
      }
    }

  return rc;
}

template<typename TImageType, typename TCoordRep>
void
GaussianInterpolateImageFunction<TImageType, TCoordRep>
::ComputeErrorFunctionArray( unsigned int dimension, RealType cindex,
  vnl_vector<RealType> &erfArray, vnl_vector<RealType> &gerfArray,
  bool evaluateGradient ) const
{
  // Determine the range of voxels along the line where to evaluate erf
  int boundingBoxSize = static_cast<int>(
    this->m_BoundingBoxEnd[dimension] - this->m_BoundingBoxStart[dimension] +
    0.5 );
  int begin = vnl_math_max( 0, static_cast<int>( std::floor( cindex -
    this->m_BoundingBoxStart[dimension] -
    this->m_CutoffDistance[dimension] ) ) );
  int end = vnl_math_min( boundingBoxSize, static_cast<int>( std::ceil( cindex -
    this->m_BoundingBoxStart[dimension] +
    this->m_CutoffDistance[dimension] ) ) );

  erfArray.set_size( boundingBoxSize ); //ME: arrays are bigger than required!? cf below from line 227
  gerfArray.set_size( boundingBoxSize );

  // Start at the first voxel
  RealType t = ( this->m_BoundingBoxStart[dimension] - cindex +
    static_cast<RealType>( begin ) ) * this->m_ScalingFactor[dimension];
  // ME: vnl_erf(t) = (2/sqrt(pi)) Integral from 0 to t (exp(-x^2) dx)
  RealType e_last = vnl_erf( t );   // ME: error function
  RealType g_last = 0.0;            // ME: gradient
  if( evaluateGradient )
    {
    g_last = vnl_math::two_over_sqrtpi * std::exp( -vnl_math_sqr( t ) );
    }

  // ME: Computation of (standard) Gaussian weights (up to a constant)
  // ME: of each node given by spacing
  for( int i = begin; i < end; i++ )
    {
    t += this->m_ScalingFactor[dimension];
    RealType e_now = vnl_erf( t );
    erfArray[i] = e_now - e_last;
    if( evaluateGradient )
      {
      RealType g_now = vnl_math::two_over_sqrtpi * std::exp( -vnl_math_sqr( t ) );
      gerfArray[i] = g_now - g_last;
      g_last = g_now;
      }
    e_last = e_now;
    }
}

} // namespace itk

#endif
