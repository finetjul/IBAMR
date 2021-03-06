// Filename: INSHierarchyIntegrator.h
// Created on 02 Apr 2004 by Boyce Griffith
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

#ifndef included_INSHierarchyIntegrator
#define included_INSHierarchyIntegrator

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBAMR INCLUDES
#include <ibamr/AdvDiffHierarchyIntegrator.h>
#include <ibamr/CCHierarchyProjector.h>
#include <ibamr/GodunovAdvector.h>
#include <ibamr/HierarchyProjector.h>
#include <ibamr/INSIntermediateVelocityBcCoef.h>
#include <ibamr/INSProjectionBcCoef.h>
#include <ibamr/ibamr_enums.h>

// IBTK INCLUDES
#include <ibtk/CCPoissonFACOperator.h>
#include <ibtk/CartGridFunction.h>
#include <ibtk/FACPreconditioner.h>
#include <ibtk/LinearSolver.h>
#include <ibtk/HierarchyMathOps.h>

// SAMRAI INCLUDES
#include <CellVariable.h>
#include <CoarsenAlgorithm.h>
#include <CoarsenSchedule.h>
#include <ComponentSelector.h>
#include <FaceVariable.h>
#include <Geometry.h>
#include <GriddingAlgorithm.h>
#include <HierarchyCellDataOpsReal.h>
#include <HierarchyFaceDataOpsReal.h>
#include <LocationIndexRobinBcCoefs.h>
#include <NodeVariable.h>
#include <PatchHierarchy.h>
#include <PatchLevel.h>
#include <PoissonSpecifications.h>
#include <RefineAlgorithm.h>
#include <RefineSchedule.h>
#include <RobinBcCoefStrategy.h>
#include <SAMRAIVectorReal.h>
#include <StandardTagAndInitStrategy.h>
#include <VariableContext.h>
#include <VisItDataWriter.h>
#include <tbox/Array.h>
#include <tbox/Database.h>
#include <tbox/Pointer.h>
#include <tbox/Serializable.h>

// C++ STDLIB INCLUDES
#include <list>
#include <map>
#include <ostream>
#include <string>
#include <vector>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class INSHierarchyIntegrator manages the spatial discretization and
 * time integration of the incompressible Navier-Stokes equations, as well as
 * any scalar- and vector-valued quantities whose dynamics are specified by the
 * associated advection-diffusion equation.
 *
 * This class integrates the incompressible Navier-Stokes equations in time via
 * an second-order accurate cell-centered approximate projection method.
 * Optional time dependent forcing terms and divergence specifications may be
 * registered with the integrator.
 *
 * This integrator employs adaptive local spatial refinement.  All levels of the
 * patch hierarchy are synchronously integrated in time.  In particular,
 * subcycling in time is not employed.
 *
 * The viscous terms are treated by the AdvDiffHierarchyIntegrator object
 * supplied to the class constructor, and the advective terms are discretized by
 * the GodunovAdvector object supplied to the class constructor.
 *
 * \see AdvDiffHierarchyIntegrator
 * \see GodunovAdvector
 * \see SAMRAI::algs::HyperbolicLevelIntegrator
 * \see SAMRAI::mesh::StandardTagAndInitStrategy
 * \see SAMRAI::algs::TimeRefinementIntegrator
 * \see SAMRAI::algs::TimeRefinementLevelStrategy
 */
class INSHierarchyIntegrator
    : public SAMRAI::mesh::StandardTagAndInitStrategy<NDIM>,
      public SAMRAI::tbox::Serializable
{
public:
    /*!
     * The constructor for INSHierarchyIntegrator sets some default values,
     * reads in configuration information from input and restart databases, and
     * registers the integrator object with the restart manager when requested.
     *
     * When assertion checking is active, passing in any null pointer or an
     * empty string will result in an unrecoverable exception, except for the
     * \em optional cell-centered exact projection operator.
     */
    INSHierarchyIntegrator(
        const std::string& object_name,
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        SAMRAI::tbox::Pointer<GodunovAdvector> explicit_predictor,
        SAMRAI::tbox::Pointer<AdvDiffHierarchyIntegrator> adv_diff_hier_integrator,
        SAMRAI::tbox::Pointer<HierarchyProjector> hier_projector,
        SAMRAI::tbox::Pointer<CCHierarchyProjector> cc_hier_projector=SAMRAI::tbox::Pointer<CCHierarchyProjector>(NULL),
        bool register_for_restart=true);

    /*!
     * The destructor for INSHierarchyIntegrator unregisters the integrator
     * object with the restart manager when so registered.
     */
    ~INSHierarchyIntegrator();

    /*!
     * Return the name of the hierarchy integrator object.
     */
    const std::string&
    getName() const;

    /*!
     * Supply initial conditions for the (cell centered) velocity.
     */
    void
    registerVelocityInitialConditions(
        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> U_init);

    /*!
     * Supply physical boundary conditions for the (cell centered) velocity.
     */
    void
    registerVelocityPhysicalBcCoefs(
        const blitz::TinyVector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*,NDIM>& U_bc_coefs);

    /*!
     * Supply initial conditions for the (cell centered) pressure.
     *
     * \note These initial conditions are used for output purposes only.  They
     * are not actually used in the computation.
     */
    void
    registerPressureInitialConditions(
        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> P_init);

    /*!
     * Supply physical boundary conditions for the (cell centered) pressure.
     */
    void
    registerPressurePhysicalBcCoef(
        SAMRAI::solv::RobinBcCoefStrategy<NDIM>* const P_bc_coef);

    /*!
     * Supply an optional cell centered body force specification object.
     */
    void
    registerBodyForceSpecification(
        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> F_fcn);

    /*!
     * Supply an optional cell centered source/sink specification object.
     */
    void
    registerSourceSpecification(
        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> Q_fcn);

    /*!
     * Register a VisIt data writer so this object will write plot files that
     * may be postprocessed with the VisIt visualization tool.
     */
    void
    registerVisItDataWriter(
        SAMRAI::tbox::Pointer<SAMRAI::appu::VisItDataWriter<NDIM> > visit_writer);

    ///
    ///  The following routines:
    ///
    ///      registerRegridHierarchyCallback(),
    ///      registerApplyGradientDetectorCallback()
    ///
    ///  allow for the registration of callback functions that are executed by
    ///  the hierarchy integrator.
    ///

    /*!
     * \brief Callback registration function.
     */
    void
    registerRegridHierarchyCallback(
        void (*callback)(const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy, const double regrid_data_time, const bool initial_time, void* ctx),
        void* ctx);

    /*!
     * \brief Callback registration function.
     */
    void
    registerApplyGradientDetectorCallback(
        void (*callback)(const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy, const int level_number, const double error_data_time, const int tag_index, const bool initial_time, const bool uses_richardson_extrapolation_too, void* ctx),
        void* ctx);

    ///
    ///  The following routines:
    ///
    ///      getHierarchyMathOps(),
    ///      setHierarchyMathOps(),
    ///      isManagingHierarchyMathOps()
    ///
    ///  allow for the sharing of a single HierarchyMathOps object between
    ///  multiple HierarchyIntegrator objects.
    ///

    /*!
     * Return a pointer to the HierarchyMathOps object being used by this
     * integrator.
     */
    SAMRAI::tbox::Pointer<IBTK::HierarchyMathOps>
    getHierarchyMathOps() const;

    /*!
     * Set the HierarchyMathOps object being used by this integrator.
     *
     * When manage_ops is true, the HierarchyMathOps object is managed by the
     * integrator.  In particular, the integrator is responsible for invoking
     * HierarchyMathOps::setPatchHierarchy() and HierarchyMathOps::resetLevels()
     * following any changes to the configuration of the patch hierarchy.
     */
    void
    setHierarchyMathOps(
        SAMRAI::tbox::Pointer<IBTK::HierarchyMathOps> hier_math_ops,
        const bool manage_ops=false);

    /*!
     * Returns whether this integrator is managing the state of its
     * HierarchyMathOps object.
     *
     * When the integrator is managing the state of its HierarchyMathOps object,
     * the integrator is responsible for invoking
     * HierarchyMathOps::setPatchHierarchy() and HierarchyMathOps::resetLevels()
     * following any changes to the configuration of the patch hierarchy.
     */
    bool
    isManagingHierarchyMathOps() const;

    ///
    ///  The following routines:
    ///
    ///      initializeHierarchyIntegrator(),
    ///      initializeHierarchy(),
    ///      advanceHierarchy(),
    ///      getStableTimestep(),
    ///      atRegridPoint(),
    ///      getIntegratorTime(),
    ///      getStartTime(),
    ///      getEndTime(),
    ///      getIntegratorStep(),
    ///      getMaxIntegratorSteps(),
    ///      stepsRemaining(),
    ///      getPatchHierarchy(),
    ///      getGriddingAlgorithm(),
    ///      getGodunovAdvector(),
    ///      getAdvDiffHierarchyIntegrator(),
    ///      getHierarchyProjector(),
    ///      getCCHierarchyProjector()
    ///
    ///  allow the INSHierarchyIntegrator to be used as a hierarchy integrator.
    ///

    /*!
     * Initialize the variables and communications algorithms managed and used
     * by the integrator.
     *
     * This method must be called prior to any calls to initializeHierarchy() or
     * advanceHierarchy().  Otherwise, when assertion checking is active an
     * unrecoverable exception will occur.
     */
    void
    initializeHierarchyIntegrator(
        SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > gridding_alg);

    /*!
     * Set AMR patch hierarchy configuration and data at start of simulation.
     * If the computation is begun from a restart file, the hierarchy and data
     * are read from the hierarchy database.  Otherwise, the hierarchy and data
     * are initialized by the gridding algorithm data member.  In this case, the
     * coarsest level is constructed and initialized.  Then, error estimation is
     * performed to determine if and where it should be refined.  Successively
     * finer levels are created and initialized until the maximum allowable
     * number of levels is achieved or no further refinement is needed.  The
     * double return value is the time increment for the first data advance
     * step.
     *
     * This function assumes that the hierarchy exists, but that it contains no
     * patch levels, when it is called.  On return from this function, the
     * initial hierarchy configuration and simulation data is set properly for
     * the advanceHierarchy() function to be called.  In particular, on each
     * level constructed only the data needed for initialization exists.
     */
    double
    initializeHierarchy();

    /*!
     * Integrate data on all patches on all levels of the patch hierarchy from
     * current time (current_time) to new time (new_time).
     *
     * When assertion checking is active, an unrecoverable exception will result
     * if the new time is not greater than the given time.
     */
    double
    advanceHierarchy(
        const double dt);

    /*!
     * Returns the maximum stable timestep according to the hyperbolic CFL
     * condition and a growth condition.  The growth condition is imposed to
     * prevent excessive changes in the maximum stable timestep as the
     * computation progresses.
     */
    double
    getStableTimestep() const;

    /*!
     * Return true if the current step count indicates that regridding should
     * occur.
     */
    bool
    atRegridPoint() const;

    /*!
     * Return the current integration time for coarsest hierarchy level.
     */
    double
    getIntegratorTime() const;

    /*!
     * Return the initial integration time.
     */
    double
    getStartTime() const;

    /*!
     * Return the final integration time.
     */
    double
    getEndTime() const;

    /*!
     * Return the integration step count for entire hierarchy (i.e., number of
     * steps taken on the coarsest level).
     */
    int
    getIntegratorStep() const;

    /*!
     * Return the maximum number of integration steps allowed for entire
     * hierarchy (i.e., steps allowed on coarsest level).
     */
    int
    getMaxIntegratorSteps() const;

    /*!
     * Return true if any integration steps remain, false otherwise.
     */
    bool
    stepsRemaining() const;

    /*!
     * Return a const pointer to the patch hierarchy managed by integrator.
     */
    const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> >
    getPatchHierarchy() const;

    /*!
     * Return a pointer to the gridding algorithm object.
     */
    SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> >
    getGriddingAlgorithm() const;

    /*!
     * Return a pointer to the Godunov advector being used to predict the
     * advection velocities.
     */
    SAMRAI::tbox::Pointer<GodunovAdvector>
    getGodunovAdvector() const;

    /*!
     * Return a pointer to the AdvDiffHierarchyIntegrator being used to
     * integrate the advection-diffusion equation.
     */
    SAMRAI::tbox::Pointer<AdvDiffHierarchyIntegrator>
    getAdvDiffHierarchyIntegrator() const;

    /*!
     * Return a pointer to the HierarchyProjector being used to enforce
     * incompressibility.
     */
    SAMRAI::tbox::Pointer<HierarchyProjector>
    getHierarchyProjector() const;

    /*!
     * Return a pointer to the CCHierarchyProjector being used to enforce
     * incompressibility.
     */
    SAMRAI::tbox::Pointer<CCHierarchyProjector>
    getCCHierarchyProjector() const;

    ///
    ///  The following routines:
    ///
    ///      regridHierarchy(),
    ///      predictAdvectionVelocity(),
    ///      integrateAdvDiff(),
    ///      projectHierarchy(),
    ///      updatePressure(),
    ///      synchronizeHierarchy(),
    ///      synchronizeNewLevels(),
    ///      resetTimeDependentHierData(),
    ///      resetHierDataToPreadvanceState()
    ///
    ///  allow the INSHierarchyIntegrator to provide data management for a time
    ///  integrator which makes use of this class.
    ///

    /*!
     * Regrid the hierarchy.
     */
    void
    regridHierarchy();

    /*!
     * This routine predicts a time-centered advection velocity using an
     * explicit Godunov-like extrapolation.  This MAC advection velocity is
     * exactly projected on the composite grid to ensure that it satisfies the
     * specified divergence condition.
     *
     * This method is additionally responsible for performing a "synchronization
     * projection" following any regridding operation.
     */
    void
    predictAdvectionVelocity(
        const double current_time,
        const double new_time);

    /*!
     * This routine integrates the advection-diffusion equation for the cell
     * centered intermediate, unprojected velocity field.
     */
    void
    integrateAdvDiff(
        const double current_time,
        const double new_time);

    /*!
     * This routine approximately projects the cell centered intermediate
     * velocity field, approximately enforcing the specified divergence
     * condition.
     */
    void
    projectVelocity(
        const double current_time,
        const double new_time);

    /*!
     * This routine updates the value of the pressure.  The exact form of this
     * update may require the solution to additional systems of linear
     * equations.
     */
    void
    updatePressure(
        const double current_time,
        const double new_time,
        const bool override_current_pressure=false);

    /*!
     * Synchronize the hierarchy.
     */
    void
    synchronizeHierarchy();

    /*!
     * Coarsen current solution data from finest hierarchy level specified down
     * through the coarsest hierarchy level specified, if initial_time is true.
     * In this case, the hierarchy is being constructed at the initial
     * simulation time, After data is coarsened, the application- specific
     * initialization routine is called to set data before that solution is
     * further coarsened to the next coarser level in the hierarchy.  This
     * operation makes the solution consistent between coarser levels and finer
     * levels that did not exist when the coarse levels where created and
     * initialized originally.
     *
     * When initial_time is false, this routine does nothing since the standard
     * hyperbolic AMR algorithm for conservation laws requires no data
     * synchronization after regridding beyond interpolation of data from
     * coarser levels in the hierarchy in some conservative fashion.
     *
     * When assertion checking is active, an unrecoverable exception will result
     * if the hierarchy pointer is null, the level numbers do not properly match
     * existing levels in the hierarchy (either coarsest_level > finest_level or
     * some level is null).
     */
    void
    synchronizeNewLevels(
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int coarsest_level,
        const int finest_level,
        const double sync_time,
        const bool initial_time);

    /*!
     * Reset time dependent data.
     */
    void
    resetTimeDependentHierData(
        const double new_time);

    /*!
     * Deallocate all new simulation data.
     */
    void
    resetHierDataToPreadvanceState();

    ///
    ///  The following routines:
    ///
    ///      initializeLevelData(),
    ///      resetHierarchyConfiguration(),
    ///      applyGradientDetector()
    ///
    ///  are concrete implementations of functions declared in the
    ///  SAMRAI::mesh::StandardTagAndInitStrategy abstract base class.
    ///

    /*!
     * Initialize data on a new level after it is inserted into an AMR patch
     * hierarchy by the gridding algorithm.  The level number indicates that of
     * the new level.  The old_level pointer corresponds to the level that
     * resided in the hierarchy before the level with the specified number was
     * introduced.  If the pointer is null, there was no level in the hierarchy
     * prior to the call and the level data is set based on the user routines
     * and the simulation time.  Otherwise, the specified level replaces the old
     * level and the new level receives data from the old level appropriately
     * before it is destroyed.
     *
     * Typically, when data is set, it is interpolated from coarser levels in
     * the hierarchy.  If the data is to be set, the level number must match
     * that of the old level, if non-null.  If the old level is non-null, then
     * data is copied from the old level to the new level on regions of
     * intersection between those levels before interpolation occurs.  Then,
     * user-supplied patch routines are called to further initialize the data if
     * needed.  The boolean argument initial_time is passed into the user's
     * routines.
     *
     * The boolean argument initial_time indicates whether the level is being
     * introduced for the first time (i.e., at initialization time), or after
     * some regrid process during the calculation beyond the initial hierarchy
     * construction.  This information is provided since the initialization of
     * the data on a patch may be different in each of those circumstances.  The
     * can_be_refined boolean argument indicates whether the level is the finest
     * level allowed in the hierarchy.  This may or may not affect the data
     * initialization process depending on the problem.
     *
     * When assertion checking is active, an unrecoverable exception will result
     * if the hierarchy pointer is null, the level number does not match any
     * level in the hierarchy, or the old level number does not match the level
     * number (if the old level pointer is non-null).
     */
    void
    initializeLevelData(
        const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double init_data_time,
        const bool can_be_refined,
        const bool initial_time,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchLevel<NDIM> > old_level=SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchLevel<NDIM> >(NULL),
        const bool allocate_data=true);

    /*!
     * Reset cached communication schedules after the hierarchy has changed (for
     * example, due to regridding) and the data has been initialized on the new
     * levels.  The intent is that the cost of data movement on the hierarchy
     * will be amortized across multiple communication cycles, if possible.  The
     * level numbers indicate the range of levels in the hierarchy that have
     * changed.  However, this routine updates communication schedules every
     * level finer than and including that indexed by the coarsest level number
     * given.
     *
     * When assertion checking is active, an unrecoverable exception will result
     * if the hierarchy pointer is null, any pointer to a level in the hierarchy
     * that is coarser than the finest level is null, or the given level numbers
     * not specified properly; e.g., coarsest_level > finest_level.
     */
    void
    resetHierarchyConfiguration(
        const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
        const int coarsest_level,
        const int finest_level);

    /*!
     * Set integer tags to "one" in cells where refinement of the given level
     * should occur according to some user-supplied gradient criteria.  The
     * double time argument is the regrid time.  The integer "tag_index"
     * argument is the patch descriptor index of the cell centered integer tag
     * array on each patch in the hierarchy.  The boolean argument initial_time
     * indicates whether the level is being subject to refinement at the initial
     * simulation time.  If it is false, then the error estimation process is
     * being invoked at some later time after the AMR hierarchy was initially
     * constructed.  The boolean argument uses_richardson_extrapolation_too is
     * true when Richardson extrapolation error estimation is used in addition
     * to the gradient detector, and false otherwise.  This argument helps the
     * user to manage multiple regridding criteria.  This information is passed
     * along to the user's patch tagging routines since the application of the
     * gradient detector may be different in each case.
     *
     * When assertion checking is active, an unrecoverable exception will result
     * if the hierarchy pointer is null or the level number does not match any
     * existing level in the hierarchy.
     */
    void
    applyGradientDetector(
        const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double error_data_time,
        const int tag_index,
        const bool initial_time,
        const bool uses_richardson_extrapolation_too);

    ///
    ///  The following routines:
    ///
    ///      getVelocityVar(),
    ///      getPressureVar(),
    ///      getExtrapolatedPressureVar(),
    ///      getAdvectionVelocityVar(),
    ///      getForceVar(),
    ///      getSourceVar()
    ///
    ///  allows access to the various state variables maintained by the
    ///  integrator.
    ///

    /*!
     * Return a pointer to the fluid velocity state variable.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> >
    getVelocityVar() const;

    /*!
     * Return a pointer to the fluid pressure state variable.
     *
     * \note The pressure state variable is defined at time level n-1/2.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> >
    getPressureVar() const;

    /*!
     * Return a pointer to the fluid pressure variable extrapolated forward in
     * time to time level n.
     *
     * \note The pressure state variable is defined at time level n-1/2.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> >
    getExtrapolatedPressureVar() const;

    /*!
     * Return a pointer to the advection velocity variable.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM,double> >
    getAdvectionVelocityVar() const;

    /*!
     * Return a pointer to the body force variable.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> >
    getForceVar() const;

    /*!
     * Return a pointer to the source strength variable.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> >
    getSourceVar() const;

    ///
    ///  The following routines:
    ///
    ///      getCurrentContext(),
    ///      getNewContext(),
    ///      getScratchContext()
    ///
    ///  allow access to the various variable contexts maintained by the
    ///  integrator.
    ///

    /*!
     * Return pointer to "current" variable context used by integrator.  Current
     * data corresponds to state data at the beginning of a timestep, or when a
     * new level is initialized.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext>
    getCurrentContext() const;

    /*!
     * Return pointer to "new" variable context used by integrator.  New data
     * corresponds to advanced state data at the end of a timestep.  The data is
     * one timestep later than the "current" data.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext>
    getNewContext() const;

    /*!
     * Return pointer to "scratch" variable context used by integrator.  Scratch
     * data typically corresponds to storage that user-routines in the concrete
     * GodunovAdvector object manipulate; in particular, scratch data contains
     * ghost cells.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext>
    getScratchContext() const;

    ///
    ///  The following routines:
    ///
    ///      putToDatabase()
    ///
    ///  are concrete implementations of functions declared in the
    ///  SAMRAI::tbox::Serializable abstract base class.
    ///

    /*!
     * Write out object state to the given database.
     *
     * When assertion checking is active, database pointer must be non-null.
     */
    void
    putToDatabase(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);

protected:
    /*!
     * Register a "state" variable with the integrator.  When a refine operator
     * is specified, the data for the variable is automatically maintained as
     * the patch hierarchy evolves.
     *
     * All state variables are registered with three contexts: current, new, and
     * scratch.  The current context of a state variable is maintained from
     * timestep to timestep and, optionally, as the patch hierarchy evolves.
     *
     * When a coarsen operator is specified, at the end of each timestep refined
     * regions of the new context are re-filled with the underlying fine data.
     * Whether or not a coarsen operation occurs, data in the current context is
     * then overwritten by data in the new context.
     *
     * \note If a refine operator is not specified, the data for the variable is
     * UNDEFINED following any changes to the hierarchy configuration.
     *
     * \todo Add variable registration to a HierarchyIntegrator base class.
     */
    void
    registerVariable(
        int& current_idx,
        int& new_idx,
        int& scratch_idx,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > variable,
        const SAMRAI::hier::IntVector<NDIM>& scratch_ghosts=SAMRAI::hier::IntVector<NDIM>(0),
        const std::string& coarsen_name="NO_COARSEN",
        const std::string& refine_name="NO_REFINE");

    /*!
     * Register a "scratch" variable with the integrator.  This data IS NOT
     * maintained as the patch hierarchy evolves.
     *
     * All scratch variables are registered with the scratch context.
     *
     * \todo Add variable registration to a HierarchyIntegrator base class.
     */
    void
    registerVariable(
        int& scratch_idx,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > variable,
        const SAMRAI::hier::IntVector<NDIM>& ghosts=SAMRAI::hier::IntVector<NDIM>(0));

private:
    typedef std::map<std::string,SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineAlgorithm<NDIM> > >              RefineAlgMap;
    typedef std::map<std::string,SAMRAI::xfer::RefinePatchStrategy<NDIM>* >                                 RefinePatchStrategyMap;
    typedef std::map<std::string,std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineSchedule<NDIM> > > > RefineSchedMap;

    typedef std::map<std::string,SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenAlgorithm<NDIM> > >              CoarsenAlgMap;
    typedef std::map<std::string,SAMRAI::xfer::CoarsenPatchStrategy<NDIM>* >                                 CoarsenPatchStrategyMap;
    typedef std::map<std::string,std::vector<SAMRAI::tbox::Pointer<SAMRAI::xfer::CoarsenSchedule<NDIM> > > > CoarsenSchedMap;

    /*!
     * \brief Default constructor.
     *
     * \note This constructor is not implemented and should not be used.
     */
    INSHierarchyIntegrator();

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    INSHierarchyIntegrator(
        const INSHierarchyIntegrator& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    INSHierarchyIntegrator&
    operator=(
        const INSHierarchyIntegrator& that);

    /*!
     * Compute the appropriate source term which must be added to the momentum
     * equation when the fluid contains sources and sinks.
     */
    void
    computeDivSourceTerm(
        const int F_idx,
        const int Q_idx,
        const int u_idx,
        const int coarsest_ln,
        const int finest_ln);

    /*!
     * Read input values, indicated above, from given database.  The boolean
     * argument is_from_restart should be set to true if the simulation is
     * beginning from restart.  Otherwise it should be set to false.
     *
     * When assertion checking is active, the database pointer must be non-null.
     */
    void
    getFromInput(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db,
        bool is_from_restart);

    /*!
     * Read object state from the restart file and initialize class data
     * members.  The database from which the restart data is read is determined
     * by the object_name specified in the constructor.
     *
     * Unrecoverable Errors:
     *
     *    -   The database corresponding to object_name is not found in the
     *        restart file.
     *
     *    -   The class version number and restart version number do not match.
     *
     */
    void
    getFromRestart();

    /*
     * The object name is used as a handle to databases stored in restart files
     * and for error reporting purposes.  The boolean is used to control restart
     * file writing operations.
     */
    std::string d_object_name;
    bool d_registered_for_restart;

    /*
     * Pointers to the patch hierarchy and gridding algorithm objects associated
     * with this time integration object.
     *
     * The gridding algorithm provides grid generation and regridding routines
     * for the AMR hierarchy.
     */
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > d_hierarchy;
    SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> > d_gridding_alg;

    /*
     * We cache a pointer to the VisIt data writer to register plot variables.
     *
     * Double precision values are (optional) factors used to rescale the
     * pressure, force, and source/sink density for plotting.
     */
    SAMRAI::tbox::Pointer<SAMRAI::appu::VisItDataWriter<NDIM> > d_visit_writer;
    double d_P_scale, d_F_scale, d_Q_scale;

    /*
     * The GodunovAdvector provides the numerical routines necessary to perform
     * the explicit prediction of the time and face centered advection velocity.
     */
    SAMRAI::tbox::Pointer<GodunovAdvector> d_explicit_predictor;

    /*
     * The AdvDiffHierarchyIntegrator maintains the linear solvers and related
     * data needed to handle the implicit integration of the diffusive terms and
     * the explicit integration of the advective terms.
     */
    SAMRAI::tbox::Pointer<AdvDiffHierarchyIntegrator> d_adv_diff_hier_integrator;

    /*
     * The SAMRAI::algs::HyperbolicLevelIntegrator supplies generic operations
     * needed to handle the explicit integration of advection terms.  It is
     * supplied and maintained by the AdvDiffHierarchyIntegrator.
     */
    SAMRAI::tbox::Pointer<SAMRAI::algs::HyperbolicLevelIntegrator<NDIM> > d_hyp_level_integrator;

    /*
     * The HierarchyProjector maintains the linear solvers and related data
     * needed to enforce the incompressibility constraint exactly for MAC vector
     * fields and approximately for cell-centered vector fields.
     */
    SAMRAI::tbox::Pointer<HierarchyProjector> d_hier_projector;

    /*
     * The CCHierarchyProjector maintains the linear solvers and related data
     * needed to enforce the incompressibility constraint exactly for
     * cell-centered vector fields.
     */
    SAMRAI::tbox::Pointer<CCHierarchyProjector> d_cc_hier_projector;

    /*
     * Integrator data read from input or set at initialization.
     */
    double d_start_time;
    double d_end_time;
    double d_grow_dt;
    int d_max_integrator_steps;

    /*
     * The number of cycles to perform each timestep.  During each cycle, the
     * most recently available pressure is employed as the time centered
     * approximation to the pressure gradient that appears in the momentum
     * equation.  (During the first cycle, this is simply the lagged pressure
     * computed during the previous timestep.)  Typically, num_cycles will equal
     * 1.
     */
    int d_num_cycles;

    /*
     * The number of cycles to perform during the first timestep in order to
     * initialize the pressure.
     */
    int d_num_init_cycles;

    /*
     * The regrid interval indicates the number of integration steps taken
     * between invocations of the regridding process.
     *
     * The regrid mode indicates whether to use "standard" regridding (grid
     * generation involves only one call to
     * SAMRAI::mesh::GriddingAlgorithm::regridAllFinerLevels()) or "agressive"
     * regridding (grid generation involes multiple calls to
     * SAMRAI::mesh::GriddingAlgorithm::regridAllFinerLevels()).
     */
    int d_regrid_interval;
    RegridMode d_regrid_mode;

    /*
     * The tag buffer indicates the number of cells on each level by which
     * tagged cells will be buffered after they have selected for refinement.
     * These values are passed into the gridding algorithm routines during
     * hierarchy construction and regridding.  The tag buffer helps to guarantee
     * that refined cells near important features in the solution will remain
     * refined until the level is regridded next.
     */
    bool d_using_default_tag_buffer;
    SAMRAI::tbox::Array<int> d_tag_buffer;

    /*
     * This boolean value determines whether a re-projection ("synchronization
     * projection") of the velocity field occurs following a regridding of the
     * patch hierarchy.
     */
    bool d_using_synch_projection;

    /*
     * This enum determines the differencing form of the convective operator.
     */
    ConvectiveDifferencingType d_convective_difference_form;

    /*
     * Tag cells based on the relative and absolute magnitudes of the local
     * vorticity.
     */
    bool d_using_vorticity_tagging;
    SAMRAI::tbox::Array<double> d_Omega_rel_thresh, d_Omega_abs_thresh;
    double d_Omega_max;

    /*
     * The types of projection methods to use for the velocity and pressure.
     *
     * NOTE: The velocity and pressure projection types may be different.
     */
    ProjectionMethodType d_velocity_projection_type, d_pressure_projection_type;
    bool d_using_hybrid_projection;

    /*
     * This boolean value determines whether the pressure update is second-order
     * accurate in time.
     *
     * The enum indicates the type of viscous timestepping scheme that is being
     * employed; its value is provided by class AdvDiffHierarchyIntegrator.
     */
    bool d_second_order_pressure_update;
    ViscousTimesteppingType d_viscous_timestepping_type;

    /*
     * This boolean value determines whether the pressure is normalized to have
     * zero mean (i.e., discrete integral) at the end of each timestep.
     */
    bool d_normalize_pressure;

    /*
     * These boolean values indicate whether to output the pressure, applied
     * force, and source strength for visualization.
     */
    bool d_output_P;
    bool d_output_F;
    bool d_output_Q;

    /*
     * This boolean value indicates whether to store the cell centered vorticity
     * (curl U) for visualization.
     */
    bool d_output_Omega;

    /*
     * These boolean values indicate whether to store the cell centered
     * divergences of U, u, and u_adv for visualization.
     */
    bool d_output_Div_U;
    bool d_output_Div_u;
    bool d_output_Div_u_adv;

    /*
     * Integrator data that evolves during time integration and maintains the
     * state of the timestep sequence over the levels in the AMR hierarchy.
     */
    double d_old_dt;
    double d_integrator_time;
    int    d_integrator_step;

    /*
     * A maximum timestep constraint over the specified time interval.
     */
    double d_dt_max;
    double d_dt_max_time_max;
    double d_dt_max_time_min;

    /*
     * Indicates whether the integrator has been initialized.
     */
    bool d_is_initialized;

    /*
     * Indicates whether the integrator should output logging messages.
     */
    bool d_do_log;

    /*
     * Indicates whether the velocity field needs to be re-projected.
     */
    bool d_reproject_after_regrid;

    /*
     * Indicates whether the integrator is attempting to initialize the pressure.
     */
    int d_cycle;
    bool d_performing_init_cycles;

    /*
     * The fluid density (rho), dynamic viscosity (mu), kinematic viscosity
     * (nu), and (optional) drag coefficient (lambda).
     *
     * \note rho_water = 1.00 g cm^-3
     *       mu_water  = 0.01 g cm^-1 s^-1
     *       nu_water  = 0.01 cm^2 s^-1
     */
    double d_rho, d_mu, d_nu, d_lambda;

    /*
     * Hierarchy operations objects.
     */
    SAMRAI::tbox::Pointer<SAMRAI::math::HierarchyCellDataOpsReal<NDIM,double> > d_hier_cc_data_ops;
    SAMRAI::tbox::Pointer<SAMRAI::math::HierarchyFaceDataOpsReal<NDIM,double> > d_hier_fc_data_ops;
    SAMRAI::tbox::Pointer<IBTK::HierarchyMathOps> d_hier_math_ops;
    bool d_is_managing_hier_math_ops;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> > d_wgt_var;
    double d_volume;

    /*
     * Communications algorithms, patch strategies, and schedules.
     */
    RefineAlgMap           d_ralgs;
    RefinePatchStrategyMap d_rstrategies;
    RefineSchedMap         d_rscheds;

    CoarsenAlgMap           d_calgs;
    CoarsenPatchStrategyMap d_cstrategies;
    CoarsenSchedMap         d_cscheds;

    SAMRAI::tbox::Pointer<SAMRAI::xfer::RefineAlgorithm<NDIM> > d_fill_after_regrid;
    SAMRAI::hier::ComponentSelector d_fill_after_regrid_bc_idxs;

    SAMRAI::tbox::Pointer<IBTK::HierarchyGhostCellInterpolation> d_V_bdry_fill_op, d_P_hier_bdry_fill_op, d_Phi_hier_bdry_fill_op, d_regrid_Phi_hier_bdry_fill_op, d_no_fill_op;

    /*
     * Objects to set initial conditions (note that the initial value of the
     * pressure is for visualization purposes only), boundary conditions, and
     * body forcing.
     */
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_U_init, d_P_init;
    SAMRAI::solv::LocationIndexRobinBcCoefs<NDIM>* d_default_U_bc_coef;
    blitz::TinyVector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*,NDIM> d_U_bc_coefs;
    blitz::TinyVector<INSIntermediateVelocityBcCoef*,NDIM> d_intermediate_U_bc_coefs;
    SAMRAI::solv::LocationIndexRobinBcCoefs<NDIM>* d_default_P_bc_coef;
    SAMRAI::solv::RobinBcCoefStrategy<NDIM>* d_P_bc_coef;
    INSProjectionBcCoef* d_Phi_bc_coef;
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_F_fcn, d_Q_fcn;

    /*
     * SAMRAI::hier::Variable lists and SAMRAI::hier::ComponentSelector objects
     * are used for data management.
     */
    std::list<SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > > d_state_variables;
    std::list<SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > > d_scratch_variables;

    std::list<SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > > d_copy_scratch_to_current_fast;
    std::list<SAMRAI::tbox::Pointer<SAMRAI::hier::Variable<NDIM> > > d_copy_scratch_to_current_slow;

    SAMRAI::hier::ComponentSelector d_current_data;
    SAMRAI::hier::ComponentSelector d_new_data;
    SAMRAI::hier::ComponentSelector d_scratch_data;

    /*!
     * State and temporary variables.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> > d_U_var, d_U_star_var, d_V_var, d_F_U_var, d_F_var, d_F_div_var, d_Q_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> > d_P_var, d_P_extrap_var, d_Grad_P_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> > d_Phi_var, d_Phi_tilde_var, d_Grad_Phi_var;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM,double> > d_u_var, d_u_adv_var;
    SAMRAI::tbox::Pointer<SAMRAI::pdat::FaceVariable<NDIM,double> > d_grad_Phi_var;

    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> > d_Omega_var;

#if (NDIM == 3)
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> > d_Omega_Norm_var;
#endif

    /*
     * Debugging (plotting) variables.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::CellVariable<NDIM,double> > d_Div_U_var, d_Div_u_var, d_Div_u_adv_var;

    /*
     * Patch data descriptor indices for all variables managed by the
     * integrator.
     *
     * State variables have three contexts: current, scratch, and new.
     */
    int d_u_current_idx, d_u_new_idx, d_u_scratch_idx;

    int d_U_star_current_idx,    d_U_star_new_idx,    d_U_star_scratch_idx;
    int d_P_current_idx,         d_P_new_idx,         d_P_scratch_idx;
    int d_P_extrap_current_idx,  d_P_extrap_new_idx,  d_P_extrap_scratch_idx;
    int d_Phi_current_idx,       d_Phi_new_idx,       d_Phi_scratch_idx;
    int d_Phi_tilde_current_idx, d_Phi_tilde_new_idx, d_Phi_tilde_scratch_idx;
    int d_F_current_idx,         d_F_new_idx,         d_F_scratch_idx;
    int d_Q_current_idx,         d_Q_new_idx,         d_Q_scratch_idx;

    int d_F_div_current_idx, d_F_div_new_idx, d_F_div_scratch_idx;
    int d_Omega_current_idx, d_Omega_new_idx, d_Omega_scratch_idx;

#if (NDIM == 3)
    int d_Omega_Norm_idx;
#endif

    int d_Div_U_current_idx,     d_Div_U_new_idx,     d_Div_U_scratch_idx;
    int d_Div_u_current_idx,     d_Div_u_new_idx,     d_Div_u_scratch_idx;
    int d_Div_u_adv_current_idx, d_Div_u_adv_new_idx, d_Div_u_adv_scratch_idx;

    /*
     * Patch data descriptor indices for all variables managed by the
     * integrator.
     *
     * Scratch variables have only one context.
     */
    int d_Grad_P_idx, d_Grad_Phi_idx, d_grad_Phi_idx, d_V_idx;

    /*
     * Patch data descriptors for all variables managed by the
     * AdvDiffHierarchyIntegrator class.
     *
     * TIME_DEP variables have three contexts: current, scratch, and new.  Note
     * that the new context is only available for use after the
     * advection-diffusion equation has been solved.
     *
     * NO_FILL variables have only one context.
     */
    int d_U_current_idx,     d_U_new_idx,     d_U_scratch_idx;
    int d_F_U_current_idx,   d_F_U_new_idx,   d_F_U_scratch_idx;
    int d_u_adv_current_idx, d_u_adv_new_idx, d_u_adv_scratch_idx;

    /*
     * Patch data descriptors for all variables managed by the HierarchyMathOps
     * class.
     *
     * Such variables have only one context.
     */
    int d_wgt_idx;

    int d_helmholtz_max_iterations;
    double d_helmholtz_abs_residual_tol, d_helmholtz_rel_residual_tol;
    bool d_helmholtz_using_FAC;

    SAMRAI::tbox::Pointer<IBTK::CCLaplaceOperator>    d_helmholtz_op    ;
    SAMRAI::solv::PoissonSpecifications*              d_helmholtz_spec  ;
    SAMRAI::tbox::Pointer<IBTK::KrylovLinearSolver>   d_helmholtz_solver;
    SAMRAI::tbox::Pointer<IBTK::CCPoissonFACOperator> d_helmholtz_fac_op;
    SAMRAI::tbox::Pointer<IBTK::FACPreconditioner>    d_helmholtz_fac_pc;
    bool d_helmholtz_solver_needs_init;

    SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> d_fac_op_db, d_fac_pc_db;

    /*!
     * \brief Callback function pointers and callback contexts.
     */
    std::vector<void (*)(const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy, const double regrid_data_time, const bool initial_time, void* ctx)> d_regrid_hierarchy_callbacks;
    std::vector<void*> d_regrid_hierarchy_callback_ctxs;

    std::vector<void (*)(const SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy, const int level_number, const double error_data_time, const int tag_index, const bool initial_time, const bool uses_richardson_extrapolation_too, void* ctx)> d_apply_gradient_detector_callbacks;
    std::vector<void*> d_apply_gradient_detector_callback_ctxs;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/INSHierarchyIntegrator.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_INSHierarchyIntegrator
