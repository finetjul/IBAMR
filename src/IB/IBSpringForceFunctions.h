// Filename: IBSpringForceFunctions.h
// Created on 14 Jul 2004 by Boyce Griffith
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

#ifndef included_IBSpringForceFunctions
#define included_IBSpringForceFunctions

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBAMR INCLUDES
#include <ibamr/IBLagrangianForceStrategy.h>

// PETSc INCLUDES
#include <petscmat.h>

// C++ STDLIB INCLUDES
#include <limits>
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{

/*!
 * \brief Typedef specifying the spring force function API.
 */
typedef
void
(*SpringForceFcnPtr)(
    double* restrict F,
    const double* restrict D,
    double stf,
    double rst,
    int lag_mastr_idx,
    int lag_slave_idx);

/*!
 * \brief Function to compute the force generated by a linear spring with either
 * a zero or a non-zero resting length.  Considering a single spring, if \f$ k
 * \f$ is the index of the "master" node and \f$ l \f$ is the index of the
 * "slave" node, the forces generated are \f[
 *
 *      \vec{F}_k = \kappa_{k,l} \left( 1 - \frac{r_{k,l}}{\|\vec{X}_{l} - \vec{X}_{k}\|} \right) \left(\vec{X}_{l} - \vec{X}_{k}\right)
 *
 * \f] and \f[
 *
 *      \vec{F}_l = \kappa_{k,l} \left( 1 - \frac{r_{k,l}}{\|\vec{X}_{l} - \vec{X}_{k}\|} \right) \left(\vec{X}_{k} - \vec{X}_{l}\right) = - \vec{F}_k
 *
 * \f] where \f$ \kappa_{k,l} \f$ is the stiffness of the spring connecting
 * nodes \f$ k \f$ and \f$ l \f$, and where \f$ r_{k,l} \f$ is the associated
 * resting length of the spring.
 *
 * \param F              The force associated with the "master" node.
 * \param D              The displacement between the "master" and "slave" nodes, i.e., \f$ \vec{X}_{l} - \vec{X}_{k} \f$.
 * \param stf            The stiffness of the spring.
 * \param rst            The resting length of the spring (possibly zero).
 * \param lag_mastr_idx  The Lagrangian index of the "master" node associated with the spring.
 * \param lag_slave_idx  The Lagrangian index of the "slave" node associated with the spring.
 *
 * \note This is the default force generation function employed by class
 * IBSpringForceGen.  It is associated with \a force_fcn_idx 0.  Users may
 * override this default value with any function that implements the interface
 * required by IBSpringForceGen::registerSpringForceFunction().
 */
inline void
default_linear_spring_force(
    double* restrict F,
    const double* restrict D,
    double stf,
    double rst,
    int /*lag_mastr_idx*/,
    int /*lag_slave_idx*/)
{
    /*
     * Compute the distance between the "master" and "slave" nodes.
     */
    const double R = std::sqrt(std::inner_product(D,D+NDIM,D,0.0));

    /*
     * Compute the force applied to the "master" node.
     */
    const double& kappa = stf;
    const double& R0 = rst;
    if (R > std::numeric_limits<double>::epsilon())
    {
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            F[d] = kappa*(1.0-(R0/R))*D[d];
        }
    }
    else
    {
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            F[d] = kappa*D[d];
        }
    }
    return;
}// default_linear_spring_force

}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBSpringForceFunctions.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBSpringForceFunctions
