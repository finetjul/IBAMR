// Filename: CCPoissonHypreLevelSolver.h
// Created on 30 May 2005 by Boyce Griffith
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

#ifndef included_CCPoissonHypreLevelSolver
#define included_CCPoissonHypreLevelSolver

/////////////////////////////// INCLUDES /////////////////////////////////////

// HYPRE INCLUDES
#ifndef included_HYPRE_struct_ls
#define included_HYPRE_struct_ls
#include <HYPRE_struct_ls.h>
#endif

// IBTK INCLUDES
#include <ibtk/LinearSolver.h>

// SAMRAI INCLUDES
#include <BoundaryBox.h>
#include <Box.h>
#include <CellData.h>
#include <LocationIndexRobinBcCoefs.h>
#include <OutersideData.h>
#include <Patch.h>
#include <PatchHierarchy.h>
#include <PoissonSpecifications.h>
#include <RobinBcCoefStrategy.h>
#include <SAMRAIVectorReal.h>
#include <SideData.h>
#include <tbox/Array.h>
#include <tbox/Database.h>
#include <tbox/Pointer.h>

// C++ STDLIB INCLUDES
#include <ostream>
#include <string>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBTK
{
/*!
 * \brief Class CCPoissonHypreLevelSolver is a concrete LinearSolver for solving
 * elliptic equations of the form \f$ \mbox{$L u$} = \mbox{$(C I + \nabla \cdot
 * D \nabla) u$} = f \f$ on a \em single SAMRAI::hier::PatchLevel using <A
 * HREF="http://www.llnl.gov/CASC/linear_solvers">hypre</A>.
 *
 * This solver class uses the \em hypre library to solve linear equations of the
 * form \f$ (C I + \nabla \cdot D \nabla ) u = f \f$, where \f$C\f$ is a
 * cell-centered array, \f$D\f$ is a side-centered array, and \f$u\f$ and
 * \f$f\f$ are cell-centered arrays.  The discretization is second-order
 * accurate.
 *
 * Robin boundary conditions may be specified through the interface class
 * SAMRAI::solv::RobinBcCoefStrategy.
 *
 * The user must perform the following steps to use class
 * CCPoissonHypreLevelSolver:
 *
 * -# Create a CCPoissonHypreLevelSolver object.
 * -# Set the problem specification via setPoissonSpecifications(),
 *    setPhysicalBcCoef(), and setHomogeneousBc().
 * -# Initialize CCPoissonHypreLevelSolver object using the function
 *    initializeSolverState().
 * -# Solve the linear system using the member function solveSystem(), passing
 *    in SAMRAI::solv::SAMRAIVectorReal objects corresponding to \f$u\f$ and
 *    \f$f\f$.
 *
 * Sample parameters for initialization from database (and their default
 * values): \verbatim

 enable_logging = FALSE         // see enableLogging()
 solver_type = "PFMG"           // choices are: "PFMG", "SMG", "PCG", "GMRES", "FlexGMRES" , "LGMRES", "BiCGSTAB"
 precond_type = "none"          // choices are: "PFMG", "SMG", "Jacobi", "none"
 max_iterations = 10            // see setMaxIterations()
 absolute_residual_tol = 0.0    // see setAbsoluteTolerance() (only used by hypre Krylov solvers)
 relative_residual_tol = 1.0e-6 // see setRelativeTolerance()
 rel_change = 0                 // see hypre User's Manual
 num_pre_relax_steps = 1        // number of pre-sweeps (only used by SMG or PFMG solver or preconditioner)
 num_post_relax_steps = 1       // number of post-sweeps (only used by SMG or PFMG solver or preconditioner)
 memory_use = 0                 // see hypre User's Manual (only used by SMG solver or preconditioner)
 rap_type = 0                   // see hypre User's Manual (only used by PFMG solver or preconditioner)
 relax_type = 1                 // see hypre User's Manual (only used by PFMG solver or preconditioner)
 skip_relax = 1                 // see hypre User's Manual (only used by PFMG solver or preconditioner)
 two_norm = 1                   // see hypre User's Manual (only used by PCG solver)
 \endverbatim
 *
 * \em hypre is developed in the Center for Applied Scientific Computing (CASC)
 * at Lawrence Livermore National Laboratory (LLNL).  For more information about
 * \em hypre, see <A HREF="http://www.llnl.gov/CASC/linear_solvers">
 * http://www.llnl.gov/CASC/linear_solvers</A>.
 */
class CCPoissonHypreLevelSolver
    : public LinearSolver
{
public:
    /*!
     * \brief Constructor.
     *
     * \param object_name  Name of object.
     * \param input_db     Optional SAMRAI::tbox::Database for input.
     */
    CCPoissonHypreLevelSolver(
        const std::string& object_name,
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db=NULL);

    /*!
     * \brief Destructor.
     */
    ~CCPoissonHypreLevelSolver();

    /*!
     * \name Functions for specifying the Poisson problem.
     */
    //\{

    /*!
     * \brief Set the scalar Poisson equation specifications.
     */
    void
    setPoissonSpecifications(
        const SAMRAI::solv::PoissonSpecifications& poisson_spec);

    /*!
     * \brief Set the SAMRAI::solv::RobinBcCoefStrategy object used to specify
     * physical boundary conditions.
     *
     * \note \a bc_coef may be NULL.  In this case, homogeneous Dirichlet
     * boundary conditions are employed.
     *
     * \param bc_coef  Pointer to an object that can set the Robin boundary condition coefficients
     */
    void
    setPhysicalBcCoef(
        SAMRAI::solv::RobinBcCoefStrategy<NDIM>* bc_coef);

    /*!
     * \brief Specify whether the boundary conditions are homogeneous.
     */
    void
    setHomogeneousBc(
        bool homogeneous_bc);

    /*!
     * \brief Set the hierarchy time, for use with the refinement schedules and
     * boundary condition routines employed by the object.
     */
    void
    setTime(
        double time);

    /*!
     * \brief Set the data depth used for the solution and rhs data.
     */
    void
    setDataDepth(
        int depth);

    //\}

    /*!
     * \name Linear solver functionality.
     */
    //\{

    /*!
     * \brief Solve the linear system of equations \f$Ax=b\f$ for \f$x\f$.
     *
     * Before calling solveSystem(), the form of the solution \a x and
     * right-hand-side \a b vectors must be set properly by the user on all
     * patch interiors on the specified range of levels in the patch hierarchy.
     * The user is responsible for all data management for the quantities
     * associated with the solution and right-hand-side vectors.  In particular,
     * patch data in these vectors must be allocated prior to calling this
     * method.
     *
     * \param x solution vector
     * \param b right-hand-side vector
     *
     * <b>Conditions on Parameters:</b>
     * - vectors \a x and \a b must have same patch hierarchy
     * - vectors \a x and \a b must have same structure, depth, etc.
     *
     * \note The vector arguments for solveSystem() need not match those for
     * initializeSolverState().  However, there must be a certain degree of
     * similarity, including:\par
     * - hierarchy configuration (hierarchy pointer and range of levels)
     * - number, type and alignment of vector component data
     * - ghost cell widths of data in the solution \a x and right-hand-side \a b
     *   vectors
     *
     * \note The solver need not be initialized prior to calling solveSystem();
     * however, see initializeSolverState() and deallocateSolverState() for
     * opportunities to save overhead when performing multiple consecutive
     * solves.
     *
     * \see initializeSolverState
     * \see deallocateSolverState
     *
     * \return \p true if the solver converged to the specified tolerances, \p
     * false otherwise
     */
    bool
    solveSystem(
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& x,
        SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& b);

    /*!
     * \brief Compute hierarchy dependent data required for solving \f$Ax=b\f$.
     *
     * By default, the solveSystem() method computes some required hierarchy
     * dependent data before solving and removes that data after the solve.  For
     * multiple solves that use the same hierarchy configuration, it is more
     * efficient to:
     *
     * -# initialize the hierarchy-dependent data required by the solver via
     *    initializeSolverState(),
     * -# solve the system one or more times via solveSystem(), and
     * -# remove the hierarchy-dependent data via deallocateSolverState().
     *
     * Note that it is generally necessary to reinitialize the solver state when
     * the hierarchy configuration changes.
     *
     * \param x solution vector
     * \param b right-hand-side vector
     *
     * <b>Conditions on Parameters:</b>
     * - vectors \a x and \a b must have same patch hierarchy
     * - vectors \a x and \a b must have same structure, depth, etc.
     *
     * \note The vector arguments for solveSystem() need not match those for
     * initializeSolverState().  However, there must be a certain degree of
     * similarity, including:\par
     * - hierarchy configuration (hierarchy pointer and range of levels)
     * - number, type and alignment of vector component data
     * - ghost cell widths of data in the solution \a x and right-hand-side \a b
     *   vectors
     *
     * \note It is safe to call initializeSolverState() when the state is
     * already initialized.  In this case, the solver state is first deallocated
     * and then reinitialized.
     *
     * \see deallocateSolverState
     */
    void
    initializeSolverState(
        const SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& x,
        const SAMRAI::solv::SAMRAIVectorReal<NDIM,double>& b);

    /*!
     * \brief Remove all hierarchy dependent data allocated by
     * initializeSolverState().
     *
     * \note It is safe to call deallocateSolverState() when the solver state is
     * already deallocated.
     *
     * \see initializeSolverState
     */
    void
    deallocateSolverState();

    //\}

    /*!
     * \name Functions to access solver parameters.
     */
    //\{

    /*!
     * \brief Set whether the initial guess is non-zero.
     */
    void
    setInitialGuessNonzero(
        bool initial_guess_nonzero=true);

    /*!
     * \brief Get whether the initial guess is non-zero.
     */
    bool
    getInitialGuessNonzero() const;

    /*!
     * \brief Set the maximum number of iterations to use per solve.
     */
    void
    setMaxIterations(
        int max_iterations);

    /*!
     * \brief Get the maximum number of iterations to use per solve.
     */
    int
    getMaxIterations() const;

    /*!
     * \brief Set the absolute residual tolerance for convergence.
     */
    void
    setAbsoluteTolerance(
        double abs_residual_tol);

    /*!
     * \brief Get the absolute residual tolerance for convergence.
     */
    double
    getAbsoluteTolerance() const;

    /*!
     * \brief Set the relative residual tolerance for convergence.
     */
    void
    setRelativeTolerance(
        double rel_residual_tol);

    /*!
     * \brief Get the relative residual tolerance for convergence.
     */
    double
    getRelativeTolerance() const;

    //\}

    /*!
     * \name Functions to access data on the most recent solve.
     */
    //\{

    /*!
     * \brief Return the iteration count from the most recent linear solve.
     */
    int
    getNumIterations() const;

    /*!
     * \brief Return the residual norm from the most recent iteration.
     */
    double
    getResidualNorm() const;

    //\}

    /*!
     * \name Logging functions.
     */
    //\{

    /*!
     * \brief Enable or disable logging.
     */
    void
    enableLogging(
        bool enabled=true);

    //\}

private:
    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    CCPoissonHypreLevelSolver();

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    CCPoissonHypreLevelSolver(
        const CCPoissonHypreLevelSolver& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    CCPoissonHypreLevelSolver&
    operator=(
        const CCPoissonHypreLevelSolver& that);

    /*!
     * \brief Functions to allocate, initialize, access, and deallocate hypre
     * data structures.
     */
    void
    allocateHypreData();
    void
    setMatrixCoefficients_aligned();
    void
    setMatrixCoefficients_nonaligned();
    void
    setupHypreSolver();
    bool
    solveSystem(
        int x_idx,
        int b_idx);
    void
    copyToHypre(
        HYPRE_StructVector vector,
        SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM,double> > src_data,
        const SAMRAI::hier::Box<NDIM>& box);
    void
    copyFromHypre(
        SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM,double> > dst_data,
        HYPRE_StructVector vector,
        const SAMRAI::hier::Box<NDIM>& box);
    void
    destroyHypreSolver();
    void
    deallocateHypreData();

    /*!
     * \brief Adjust the rhs to account for inhomogeneous boundary conditions in
     * the case of isotropic or grid-aligned anisotropic problems.
     */
    void
    adjustBoundaryRhsEntries_aligned(
        SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM,double> > rhs_data,
        SAMRAI::tbox::Pointer<SAMRAI::pdat::OutersideData<NDIM,double> > D_data,
        SAMRAI::tbox::Pointer<SAMRAI::hier::Patch<NDIM> > patch,
        const SAMRAI::tbox::Array<SAMRAI::hier::BoundaryBox<NDIM> >& surface_boxes,
        const double* dx);

    /*!
     * \brief Adjust the rhs to account for inhomogeneous boundary conditions in
     * the case of non-grid-aligned anisotropic problems.
     */
    void
    adjustBoundaryRhsEntries_nonaligned(
        SAMRAI::tbox::Pointer<SAMRAI::pdat::CellData<NDIM,double> > rhs_data,
        SAMRAI::tbox::Pointer<SAMRAI::pdat::OutersideData<NDIM,double> > D_data,
        SAMRAI::tbox::Pointer<SAMRAI::hier::Patch<NDIM> > patch,
        const SAMRAI::tbox::Array<SAMRAI::hier::BoundaryBox<NDIM> >& surface_boxes,
        const double* dx);

    /*!
     * \brief Object name.
     */
    std::string d_object_name;

    /*!
     * \brief Solver initialization status.
     */
    bool d_is_initialized;

    /*!
     * \brief Associated hierarchy.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > d_hierarchy;

    /*!
     * \brief Associated level number.
     *
     * Currently, this must be level number 0.
     */
    int d_level_num;

    /*!
     * \name Problem specification and boundary condition handling.
     */
    //\{
    SAMRAI::solv::PoissonSpecifications d_poisson_spec;
    bool d_grid_aligned_anisotropy;

    /*!
     * \brief Robin boundary coefficient object for physical boundaries and
     * related data.
     */
    SAMRAI::solv::LocationIndexRobinBcCoefs<NDIM>* const d_default_bc_coef;
    SAMRAI::solv::RobinBcCoefStrategy<NDIM>* d_bc_coef;
    bool d_homogeneous_bc;
    double d_apply_time;
    int d_depth;

    //\}

    /*!
     * \name hypre objects.
     */
    //\{
    HYPRE_StructGrid    d_grid;
    HYPRE_StructStencil d_stencil;
    HYPRE_StructMatrix  d_matrix;
    HYPRE_StructVector  d_rhs_vec, d_sol_vec;
    HYPRE_StructSolver  d_solver, d_precond;

    std::string d_solver_type, d_precond_type;
    int d_max_iterations;
    double d_abs_residual_tol;
    double d_rel_residual_tol;
    bool d_initial_guess_nonzero;
    int d_rel_change;
    int d_num_pre_relax_steps, d_num_post_relax_steps;
    int d_memory_use;
    int d_rap_type;
    int d_relax_type;
    int d_skip_relax;
    int d_two_norm;

    int d_current_its;
    double d_current_residual_norm;
    //\}

    /*!
     * \name Variables for debugging and analysis.
     */
    //\{

    /*!
     * \brief Flag to print solver info.
     */
    bool d_enable_logging;

    //\}
};
}// namespace IBTK

/////////////////////////////// INLINE ///////////////////////////////////////

#include <ibtk/CCPoissonHypreLevelSolver.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_CCPoissonHypreLevelSolver
