// Filename: PETScMultiVec.h
// Created on 13 Mar 2008 by Boyce Griffith
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

#ifndef included_PETScMultiVec
#define included_PETScMultiVec

/////////////////////////////// INCLUDES /////////////////////////////////////

// PETSc INCLUDES
#include <petscvec.h>

/////////////////////////////// FUNCTION PROTOTYPES //////////////////////////

namespace IBTK
{
/*!
 * \brief Create a multi-vector PETSc Vec comprised of one or more individual <A
 * HREF="http://www-unix.mcs.anl.gov/petsc">PETSc</A> Vec objects.
 *
 * \param comm MPI communicator to associate with the MultiVec object
 * \param n number of component Vec objects
 * \param v array of component of Vec objects
 * \param vv output vector
 *
 * \note Memory associated with the component vectors is \em not freed when the
 * vector is destroyed via VecDestroy().  Users must \em not free the array
 * until the vector is destroyed.
 */
PetscErrorCode
VecCreateMultiVec(
    MPI_Comm comm,
    PetscInt n,
    Vec* v,
    Vec* vv);

/*!
 * \brief Return the number of component Vec objects stored in a MultiVec
 * vector.
 */
PetscErrorCode
VecMultiVecGetNumberOfVecs(
    Vec v,
    PetscInt* n);

/*!
 * \brief Return a pointer to the component Vec objects stored in a MultiVec
 * vector.
 *
 * \param v input vector
 * \param vv pointer to array of component vectors
 */
PetscErrorCode
VecMultiVecGetVecs(
    Vec v,
    Vec* vv[]);
}// namespace IBTK

/////////////////////////////// INLINE ///////////////////////////////////////

//#include "PETScMultiVec.I"

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_PETScMultiVec
