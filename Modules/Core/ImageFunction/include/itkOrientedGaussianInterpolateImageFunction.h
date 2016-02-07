/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkOrientedGaussianInterpolateImageFunction.h,v $
  Language:  C++
  Date:      $Date: $
  Version:   $Revision: $

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef itkOrientedGaussianInterpolateImageFunction_h
#define itkOrientedGaussianInterpolateImageFunction_h

#include "itkInterpolateImageFunction.h"

#include "itkConceptChecking.h"
#include "itkFixedArray.h"
#include "vnl/vnl_erf.h"

namespace itk
{

/** \class OrientedGaussianInterpolateImageFunction
 * \brief Evaluates the Gaussian interpolation of an image.
 *
 * This class defines an N-dimensional Gaussian interpolation function using
 * the vnl error function.  The two parameters associated with this function
 * are:
 *   1. Sigma - a scalar array of size ImageDimension determining the width
 *      of the interpolation function.
 *   2. Alpha - a scalar specifying the cutoff distance over which the function
 *      is calculated.
 *
 * This work was originally described in the Insight Journal article:
 * P. Yushkevich, N. Tustison, J. Gee, Gaussian interpolation.
 * \sa{http://hdl.handle.net/10380/3139}
 *
 * \author Paul Yushkevich
 * \author Nick Tustison
 *
 * \ingroup ITKImageFunction
 */

template <typename TInputImage, typename TCoordRep = double>
class OrientedGaussianInterpolateImageFunction :
  public InterpolateImageFunction<TInputImage, TCoordRep>
{
public:
  /** Standard class typedefs. */
  typedef OrientedGaussianInterpolateImageFunction         Self;
  typedef InterpolateImageFunction<TInputImage, TCoordRep> Superclass;
  typedef SmartPointer<Self>                               Pointer;
  typedef SmartPointer<const Self>                         ConstPointer;

  /** Run-time type information (and related methods). */
  itkTypeMacro( OrientedGaussianInterpolateImageFunction, InterpolateImageFunction );

  /** Method for creation through the object factory. */
  itkNewMacro( Self );

  /** ImageDimension constant */
  itkStaticConstMacro( ImageDimension, unsigned int,
    TInputImage::ImageDimension );


  /** OutputType typedef support. */
  typedef typename Superclass::OutputType OutputType;

  /** InputImageType typedef support. */
  typedef typename Superclass::InputImageType InputImageType;

  /** RealType typedef support. */
  typedef typename Superclass::RealType RealType;

  /** Index typedef support. */
  typedef typename Superclass::IndexType IndexType;

  /** ContinuousIndex typedef support. */
  typedef typename Superclass::ContinuousIndexType ContinuousIndexType;

  /** Array typedef support */
  typedef FixedArray<RealType, ImageDimension> ArrayType;

  /** Square array typedef support */
  typedef FixedArray<RealType, ImageDimension*ImageDimension> SquareArrayType;

  /**
   * Set input image
   */
  virtual void SetInputImage( const TInputImage *image ) ITK_OVERRIDE
    {
    Superclass::SetInputImage( image );
    this->ComputeBoundingBox();
    }

  /**
   * Set/Get sigma
   */
  virtual void SetSigma( const ArrayType s )
    {
    itkDebugMacro( "setting Sigma to " << s );
    if( this->m_Sigma != s )
      {
      this->m_Sigma = s;
      this->m_Covariance.Fill(0.0);
      for( int d = 0; d < ImageDimension; d++ )
        {
        this->m_Covariance[d*ImageDimension + d] = s[d]*s[d];
        }
      this->ComputeBoundingBox();
      this->Modified();
      }
    }
  virtual void SetSigma( RealType *s )
    {
    ArrayType sigma;
    for( unsigned int d = 0; d < ImageDimension; d++ )
      {
      sigma[d] = s[d];
      }
    this->SetSigma( sigma );
    }
  itkGetConstMacro( Sigma, ArrayType );

  /**
   * Set/Get covariance
   */
  virtual void SetCovariance( const SquareArrayType cov )
    {
    itkDebugMacro( "setting Covariance to " << cov );
    if ( this->m_Covariance != cov )
      {
      this->m_Covariance = cov;
      for( int d = 0; d < ImageDimension; d++ )
        {
        this->m_Sigma[d] = sqrt( cov[d*ImageDimension + d] );
        }
      this->ComputeBoundingBox();
      this->Modified();
      }
    }
  virtual void SetCovariance( RealType *cov )
    {
      SquareArrayType covariance;
      for( int i = 0; i < ImageDimension; i++ )
      {
        for( int j = 0; j < ImageDimension; j++ )
        {
          covariance[i*ImageDimension + j] = cov[i*ImageDimension + j];
        }
      }
      this->SetCovariance( covariance );
    }
  itkGetConstMacro( Covariance, SquareArrayType );

  /**
   * Set/Get alpha
   */
  virtual void SetAlpha( const RealType a )
    {
    itkDebugMacro( "setting Alpha to " << a );
    if( Math::NotExactlyEquals(this->m_Alpha, a) )
      {
      this->m_Alpha = a;
      this->ComputeBoundingBox();
      this->Modified();
      }
    }
  itkGetConstMacro( Alpha, RealType );

  /**
   * Set/Get sigma and alpha
   */
  virtual void SetParameters( RealType *sigma, RealType alpha )
    {
    this->SetSigma( sigma );
    this->SetAlpha( alpha );
    }

  /**
   * Evaluate at the given index
   */
  virtual OutputType EvaluateAtContinuousIndex(
    const ContinuousIndexType & cindex ) const ITK_OVERRIDE
    {
    return this->EvaluateAtContinuousIndex( cindex, ITK_NULLPTR );
    }

protected:
  OrientedGaussianInterpolateImageFunction();
  ~OrientedGaussianInterpolateImageFunction(){};
  void PrintSelf( std::ostream& os, Indent indent ) const ITK_OVERRIDE;

  virtual void ComputeBoundingBox();

  virtual RealType ComputeExponentialFunction(
    IndexType point,
    ContinuousIndexType center,
    itk::Matrix<double, ImageDimension, ImageDimension> SigmaInverse ) const;

  SquareArrayType                           m_Covariance;
  ArrayType                                 m_Sigma;
  RealType                                  m_Alpha;

  ArrayType                                 m_BoundingBoxStart;
  ArrayType                                 m_BoundingBoxEnd;
  ArrayType                                 m_CutoffDistance;

private:
  OrientedGaussianInterpolateImageFunction( const Self& ) ITK_DELETE_FUNCTION;
  void operator=( const Self& ) ITK_DELETE_FUNCTION;

  /**
   * Evaluate function value
   */
  virtual OutputType EvaluateAtContinuousIndex(
    const ContinuousIndexType &, OutputType * ) const;
};

} // end namespace itk

#ifndef ITK_MANUAL_INSTANTIATION
#include "itkOrientedGaussianInterpolateImageFunction.hxx"
#include "itkMath.h"
#endif

#endif
