// Filename: LinearOperator.h
// Created on 14 Sep 2003 by Boyce Griffith
//
// Copyright (c) 2002-2010, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of New York University nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef included_LinearOperator
#define included_LinearOperator

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBTK INCLUDES
#include <ibtk/GeneralOperator.h>

// SAMRAI INCLUDES
#include <SAMRAIVectorReal.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBTK
{
/*!
 * \brief Class LinearOperator provides an abstract interface for the
 * specification of linear operators to compute \f$ y=Ax \f$ and \f$ z=Ax+y \f$
 * and, optionally, \f$ y=A^{T} x \f$ and \f$ z=A^{T}x+y \f$.
 */
class LinearOperator
    : public GeneralOperator
{
public:
    /*!
     * \brief Default constructor.
     *
     * \param is_symmetric  Boolean that indicates whether the operator is symmetric.
     */
    LinearOperator(
        bool is_symmetric=false);

    /*!
     * \brief Empty destructor.
     */
    ~LinearOperator();

    /*!
     * \name Linear operator functionality.
     */
    //\{

    /*!
     * \brief Indicates whether the linear operator is symmetric.
     */
    bool
    isSymmetric() const;

    /*!
     * \brief Modify y to account for inhomogeneous boundary conditions.
     *
     * Before calling this function, the form of the vector y should be set
     * properly by the user on all patch interiors on the range of levels
     * covered by the operator.  All data in this vector should be allocated.
     * The user is responsible for managing the storage for the vectors.
     *
     * \note The operator MUST be initialized prior to calling
     * modifyRhsForInhomogeneousBc.
     *
     * \see initializeOperatorState
     *
     * \param y output: y=Ax
     *
     * \note A default implementation is provided which does nothing but warns
     * that inhomogeneous boundary conditions are not properly supported.
     */
    virtual void
    modifyRhsForInhomogeneousBc(
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& y);

    /*!
     * \brief Compute y=A'x.
     *
     * Before calling this function, the form of the vectors x and y should be
     * set properly by the user on all patch interiors on the range of levels
     * covered by the operator.  All data in these vectors should be allocated.
     * The user is responsible for managing the storage for the vectors.
     *
     * Conditions on arguments:
     * - vectors must have same hierarchy
     * - vectors must have same variables (except that x \em must have enough
     *   ghost cells for computation of A'x).
     *
     * In general, the vectors x and y \em cannot be the same.
     *
     * \note The operator MUST be initialized prior to calling apply.
     *
     * \see initializeOperatorState
     *
     * \param x input
     * \param y output: y=A'x
     *
     * \note A default implementation is provided.  If the operator is
     * symmetric, apply() is used.  Otherwise, the default implementation causes
     * an unrecoverable error to occur.
     */
    virtual void
    applyAdjoint(
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& x,
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& y);

    /*!
     * \brief Compute z=A'x+y.
     *
     * Before calling this function, the form of the vectors x, y, and z should
     * be set properly by the user on all patch interiors on the range of levels
     * covered by the operator.  All data in these vectors should be allocated.
     * The user is responsible for managing the storage for the vectors.
     *
     * Conditions on arguments:
     * - vectors must have same hierarchy
     * - vectors must have same variables (except that x \em must have enough
     *   ghost cells for computation of A'x).
     *
     * In general, the vectors x and z \em cannot be the same.
     *
     * \note The operator MUST be initialized prior to calling apply.
     *
     * \see initializeOperatorState
     *
     * \param x input
     * \param y input
     * \param z output: z=A'x+y
     *
     * \note A default implementation is provided which employs applyAdjoint()
     * and SAMRAI::solv::SAMRAIVectorReal::add().
     */
    virtual void
    applyAdjointAdd(
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& x,
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& y,
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& z);

    //\}

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    LinearOperator(
        const LinearOperator& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    LinearOperator&
    operator=(
        const LinearOperator& that);

    /*!
     * Indicates whether the linear operator is symmetric.
     */
    bool d_is_symmetric;
};
}// namespace IBTK

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibtk/LinearOperator.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_LinearOperator
