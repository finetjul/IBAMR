// Filename: INSStaggeredPhysicalBoundaryHelper.h
// Created on 22 Jul 2008 by Boyce Griffith
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

#ifndef included_INSStaggeredPhysicalBoundaryHelper
#define included_INSStaggeredPhysicalBoundaryHelper

/////////////////////////////// INCLUDES /////////////////////////////////////

// SAMRAI INCLUDES
#include <PatchHierarchy.h>
#include <RobinBcCoefStrategy.h>
#include <Variable.h>
#include <tbox/Pointer.h>

// BLITZ++ INCLUDES
#include <blitz/tinyvec.h>

// C++ STDLIB INCLUDES
#include <map>
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class INSStaggeredPhysicalBoundaryHelper provides various helper
 * functions required to specify physical boundary conditions for a staggered
 * grid discretization of the incompressible Navier-Stokes equations.
 */
class INSStaggeredPhysicalBoundaryHelper
    : SAMRAI::tbox::DescribedClass
{
public:
    /*!
     * \brief Default constructor.
     */
    INSStaggeredPhysicalBoundaryHelper();

    /*!
     * \brief Destructor.
     */
    ~INSStaggeredPhysicalBoundaryHelper();

    /*!
     * \brief Set values located on the physical boundary to zero on the
     * specified range of levels in the patch hierarchy using the cached
     * boundary data.
     *
     * \note By default, boundary conditions are cached over the complete range
     * of levels of the patch hierarchy.
     */
    void
    zeroValuesAtDirichletBoundaries(
        int patch_data_idx,
        int coarsest_level_number=-1,
        int finest_ln=-1) const;

    /*!
     * \brief Reset boundary values located on the physical boundary to zero on
     * the specified range of levels in the patch hierarchy using the cached
     * boundary data.
     *
     * \note By default, boundary conditions are cached over the complete range
     * of levels of the patch hierarchy.
     */
    void
    resetValuesAtDirichletBoundaries(
        int patch_data_idx,
        int coarsest_level_number=-1,
        int finest_ln=-1) const;

    /*!
     * \brief Cache boundary coefficient data.
     */
    void
    cacheBcCoefData(
        int u_idx,
        SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > u_var,
        blitz::TinyVector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*,NDIM>& u_bc_coefs,
        double fill_time,
        const SAMRAI::hier::IntVector<NDIM>& gcw_to_fill,
        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy);

    /*!
     * \brief Clear cached boundary coefficient data.
     */
    void
    clearBcCoefData();

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    INSStaggeredPhysicalBoundaryHelper(
        const INSStaggeredPhysicalBoundaryHelper& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    INSStaggeredPhysicalBoundaryHelper&
    operator=(
        const INSStaggeredPhysicalBoundaryHelper& that);

    /*!
     * Cached hierarchy-related information.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > d_hierarchy;
    std::vector<std::map<int,std::vector<SAMRAI::tbox::Pointer<SAMRAI::pdat::ArrayData<NDIM,bool> > > > > d_dirichlet_bdry_locs;
    std::vector<std::map<int,std::vector<SAMRAI::tbox::Pointer<SAMRAI::pdat::ArrayData<NDIM,double> > > > > d_dirichlet_bdry_vals;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/INSStaggeredPhysicalBoundaryHelper.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_INSStaggeredPhysicalBoundaryHelper
