// Filename: IBSpringForceGen.h
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

#ifndef included_IBSpringForceGen
#define included_IBSpringForceGen

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
    double F[NDIM],
    const double D[NDIM],
    const double& stf,
    const double& rst,
    const int& lag_mastr_idx,
    const int& lag_slave_idx)
{
    (void) lag_mastr_idx;
    (void) lag_slave_idx;

    /*
     * Compute the distance between the "master" and "slave" nodes.
     */
    double R_sq = 0.0;
    for (int d = 0; d < NDIM; ++d)
    {
        R_sq += D[d]*D[d];
    }
    const double R = sqrt(R_sq);

    /*
     * Compute the force applied to the "master" node.
     */
    const double& kappa = stf;
    const double& R0 = rst;
    if (R > std::numeric_limits<double>::epsilon())
    {
        for (int d = 0; d < NDIM; ++d)
        {
            F[d] = kappa*(1.0-(R0/R))*D[d];
        }
    }
    else
    {
        for (int d = 0; d < NDIM; ++d)
        {
            F[d] = kappa*D[d];
        }
    }
    return;
}// default_linear_spring_force

/*!
 * \brief Class IBSpringForceGen is a concrete IBLagrangianForceStrategy that
 * computes the forces generated by a collection of linear or nonlinear springs
 * (i.e., structures that resist extension and/or compression).
 *
 * \note By default, function default_linear_spring_force() is associated with
 * \a force_fcn_idx 0.  This is the default force function index.  Users may
 * override this default value with any function that implements the interface
 * required by registerSpringForceFunction().
 *
 * \note Class IBSpringForceGen DOES NOT correct for periodic displacements of
 * IB points; however, when used in conjunction with class IBStandardForceGen,
 * periodic displacements are handled correctly.
 *
 * \see IBSpringForceSpec
 */
class IBSpringForceGen
    : public IBLagrangianForceStrategy
{
public:
    /*!
     * \brief Default constructor.
     */
    IBSpringForceGen(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db=NULL);

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~IBSpringForceGen();

    /*!
     * \brief Register a spring force specification function with the force
     * generator.
     *
     * These functions are employed to compute the force generated by a
     * particular spring for the specified displacement, spring constant, rest
     * length, and Lagrangian index.
     *
     * \note By default, function default_linear_spring_force() is associated
     * with \a force_fcn_idx 0.
     */
    void
    registerSpringForceFunction(
        const int force_fcn_index,
        void (*force_fcn)(double F[NDIM], const double D[NDIM], const double& stf, const double& rst, const int& lag_mastr_idx, const int& lag_slave_idx));

    /*!
     * \brief Setup the data needed to compute the spring forces on the
     * specified level of the patch hierarchy.
     */
    virtual void
    initializeLevelData(
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double init_data_time,
        const bool initial_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the spring forces generated by the Lagrangian structure on
     * the specified level of the patch hierarchy.
     *
     * \note Nodal forces computed by this method are \em added to the force
     * vector.
     */
    virtual void
    computeLagrangianForce(
        SAMRAI::tbox::Pointer<IBTK::LMeshData> F_data,
        SAMRAI::tbox::Pointer<IBTK::LMeshData> X_data,
        SAMRAI::tbox::Pointer<IBTK::LMeshData> U_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the non-zero structure of the force Jacobian matrix.
     *
     * \note Elements indices must be global PETSc indices.
     */
    virtual void
    computeLagrangianForceJacobianNonzeroStructure(
        std::vector<int>& d_nnz,
        std::vector<int>& o_nnz,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the Jacobian of the force with respect to the present
     * structure configuration and velocity.
     *
     * \note The elements of the Jacobian should be "accumulated" in the
     * provided matrix J.
     *
     * \note A default implementation is provided with results in an assertion
     * failure for structures for which no Jacobian is available.
     */
    virtual void
    computeLagrangianForceJacobian(
        Mat& J_mat,
        MatAssemblyType assembly_type,
        const double X_coef,
        SAMRAI::tbox::Pointer<IBTK::LMeshData> X_data,
        const double U_coef,
        SAMRAI::tbox::Pointer<IBTK::LMeshData> U_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the potential energy with respect to the present structure
     * configuration and velocity.
     *
     * \note The implementation of this method will not yield the correct result
     * except for the standard force function.
     */
    virtual double
    computeLagrangianEnergy(
        SAMRAI::tbox::Pointer<IBTK::LMeshData> X_data,
        SAMRAI::tbox::Pointer<IBTK::LMeshData> U_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBSpringForceGen(
        const IBSpringForceGen& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBSpringForceGen&
    operator=(
        const IBSpringForceGen& that);

    /*!
     * \brief Read input values, indicated above, from given database.
     *
     * The database pointer may be null.
     */
    void
    getFromInput(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);

    /*!
     * \name Data maintained separately for each level of the patch hierarchy.
     */
    //\{
    std::vector<Mat> d_D_mats;
    std::vector<std::vector<int> > d_lag_mastr_node_idxs, d_lag_slave_node_idxs;
    std::vector<std::vector<int> > d_petsc_mastr_node_idxs, d_petsc_slave_node_idxs;
    std::vector<std::vector<int> > d_force_fcn_idxs;
    std::vector<std::vector<double> > d_stiffnesses, d_rest_lengths;
    std::vector<bool> d_is_initialized;
    //\}

    /*!
     * \brief Spring force functions.
     */
    std::map<int,void (*)(double F[NDIM], const double D[NDIM], const double& stf, const double& rst, const int& lag_mastr_idx, const int& lag_slave_idx)> d_force_fcn_map;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBSpringForceGen.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBSpringForceGen
