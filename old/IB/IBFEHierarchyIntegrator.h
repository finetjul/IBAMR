// Filename: IBFEHierarchyIntegrator.h
// Created on 27 Jul 2009 by Boyce Griffith
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

#ifndef included_IBFEHierarchyIntegrator
#define included_IBFEHierarchyIntegrator

/////////////////////////////// INCLUDES /////////////////////////////////////

// PETSC INCLUDES
#include <petscsys.h>

// BLITZ INCLUDES
#include <blitz/array.h>

// IBAMR INCLUDES
#include <ibamr/IBEulerianForceFunction.h>
#include <ibamr/IBLagrangianForceStrategy.h>
#include <ibamr/INSStaggeredHierarchyIntegrator.h>

// IBTK INCLUDES
#include <ibtk/FEDataManager.h>
#include <ibtk/LDataManager.h>
#include <ibtk/LMarkerSetVariable.h>

// LIBMESH INCLUDES
#define LIBMESH_REQUIRE_SEPARATE_NAMESPACE
#include <dof_map.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{
/*!
 * \brief Class IBFEHierarchyIntegrator is an implementation of a formally
 * second-order accurate, semi-implicit version of the immersed boundary method
 * which uses a Lagrangian structure defined by a libMesh mesh object to compute
 * the elastic forces.
 */
class IBFEHierarchyIntegrator
    : public SAMRAI::mesh::StandardTagAndInitStrategy<NDIM>,
      public SAMRAI::tbox::Serializable
{
public:
    static const std::string        COORDS_SYSTEM_NAME;
    static const std::string COORD_MAPPING_SYSTEM_NAME;
    static const std::string         FORCE_SYSTEM_NAME;
    static const std::string      VELOCITY_SYSTEM_NAME;
    static const std::string         J_BAR_SYSTEM_NAME;

    /*!
     * Constructor.
     *
     * When assertion checking is active, passing any null pointer or an empty
     * string as an argument will result in an assertion failure.
     */
    IBFEHierarchyIntegrator(
        const std::string& object_name,
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        SAMRAI::tbox::Pointer<INSStaggeredHierarchyIntegrator> ins_hier_integrator,
        IBTK::FEDataManager* fe_data_manager,
        IBTK::LDataManager* l_data_manager=NULL,
        bool register_for_restart=true);

    /*!
     * Constructor.
     *
     * When assertion checking is active, passing any null pointer or an empty
     * string as an argument will result in an assertion failure.
     */
    IBFEHierarchyIntegrator(
        const std::string& object_name,
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db,
        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        SAMRAI::tbox::Pointer<INSStaggeredHierarchyIntegrator> ins_hier_integrator,
        std::vector<IBTK::FEDataManager*> fe_data_managers,
        IBTK::LDataManager* l_data_manager=NULL,
        bool register_for_restart=true);

    /*!
     * Destructor.
     *
     * The destructor for IBFEHierarchyIntegrator unregisters the integrator
     * object with the restart manager when so registered.
     */
    ~IBFEHierarchyIntegrator();

    /*!
     * Return the name of the hierarchy integrator object.
     */
    const std::string&
    getName() const;

    /*!
     * Typedef specifying interface for coordinate mapping function.
     */
    typedef
    void
    (*CoordinateMappingFcnPtr)(
        libMesh::Point& X,
        const libMesh::Point& s,
        void* ctx);

    /*!
     * Register the (optional) function used to initialize the physical
     * coordinates from the Lagrangian coordinates.
     *
     * \note If no function is provided, the initial physical coordinates are
     * taken to be the same as the Lagrangian coordinate system, i.e., the
     * initial coordinate mapping is assumed to be the identity mapping.
     */
    void
    registerInitialCoordinateMappingFunction(
        CoordinateMappingFcnPtr coordinate_mapping_fcn,
        void* coordinate_mapping_fcn_ctx=NULL,
        unsigned int part=0);

    /*!
     * Typedef specifying interface for PK1 stress tensor function.
     */
    typedef
    void
    (*PK1StressFcnPtr)(
        libMesh::TensorValue<double>& PP,
        const libMesh::TensorValue<double>& FF,
        const libMesh::Point& X,
        const libMesh::Point& s,
        libMesh::Elem* elem,
        libMesh::NumericVector<double>& X_vec,
        const std::vector<libMesh::NumericVector<double>*>& system_data,
        double time,
        void* ctx);

    /*!
     * Register the function to compute the first Piola-Kirchhoff stress tensor,
     * used to compute the forces on the Lagrangian finite element mesh.
     */
    void
    registerPK1StressTensorFunction(
        PK1StressFcnPtr PK1_stress_fcn,
        std::vector<unsigned int> PK1_stress_fcn_systems=std::vector<unsigned int>(),
        void* PK1_stress_fcn_ctx=NULL,
        unsigned int part=0);

    /*!
     * Typedef specifying interface for Lagrangian body force distribution
     * function.
     */
    typedef
    void
    (*LagBodyForceFcnPtr)(
        libMesh::VectorValue<double>& F,
        const libMesh::TensorValue<double>& FF,
        const libMesh::Point& X,
        const libMesh::Point& s,
        libMesh::Elem* elem,
        libMesh::NumericVector<double>& X_vec,
        const std::vector<libMesh::NumericVector<double>*>& system_data,
        double time,
        void* ctx);

    /*!
     * Register the (optional) function to compute body force distributions on
     * the Lagrangian finite element mesh.
     */
    void
    registerLagBodyForceFunction(
        LagBodyForceFcnPtr lag_body_force_fcn,
        std::vector<unsigned int> lag_body_force_fcn_systems=std::vector<unsigned int>(),
        void* lag_body_force_fcn_ctx=NULL,
        unsigned int part=0);

    /*!
     * Typedef specifying interface for Lagrangian pressure force distribution
     * function.
     */
    typedef
    void
    (*LagPressureFcnPtr)(
        double& P,
        const libMesh::TensorValue<double>& FF,
        const libMesh::Point& X,
        const libMesh::Point& s,
        libMesh::Elem* elem,
        unsigned short int side,
        libMesh::NumericVector<double>& X_vec,
        const std::vector<libMesh::NumericVector<double>*>& system_data,
        double time,
        void* ctx);

    /*!
     * Register the (optional) function to compute surface pressure
     * distributions on the Lagrangian finite element mesh.
     */
    void
    registerLagPressureFunction(
        LagPressureFcnPtr lag_pressure_fcn,
        std::vector<unsigned int> lag_pressure_fcn_systems=std::vector<unsigned int>(),
        void* lag_pressure_fcn_ctx=NULL,
        unsigned int part=0);

    /*!
     * Typedef specifying interface for Lagrangian surface force distribution
     * function.
     */
    typedef
    void
    (*LagSurfaceForceFcnPtr)(
        libMesh::VectorValue<double>& F,
        const libMesh::TensorValue<double>& FF,
        const libMesh::Point& X,
        const libMesh::Point& s,
        libMesh::Elem* elem,
        unsigned short int side,
        libMesh::NumericVector<double>& X_vec,
        const std::vector<libMesh::NumericVector<double>*>& system_data,
        double time,
        void* ctx);

    /*!
     * Register the (optional) function to compute surface force distributions
     * on the Lagrangian finite element mesh.
     */
    void
    registerLagSurfaceForceFunction(
        LagSurfaceForceFcnPtr lag_surface_force_fcn,
        std::vector<unsigned int> lag_surface_force_fcn_systems=std::vector<unsigned int>(),
        void* lag_surface_force_fcn_ctx=NULL,
        unsigned int part=0);

    /*!
     * Register the function used to compute the forces on the Lagrangian fiber
     * mesh.
     */
    void
    registerIBLagrangianForceStrategy(
        SAMRAI::tbox::Pointer<IBLagrangianForceStrategy> ib_lag_force_strategy);

    /*!
     * Supply initial conditions for the (side centered) velocity.
     */
    void
    registerVelocityInitialConditions(
        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> U_init);

    /*!
     * Supply physical boundary conditions for the (side centered) velocity.
     */
    void
    registerVelocityPhysicalBcCoefs(
        const blitz::TinyVector<SAMRAI::solv::RobinBcCoefStrategy<NDIM>*,NDIM>& U_bc_coefs);

    /*!
     * Register an (optional) function to compute body forces on the Cartesian
     * grid.
     */
    void
    registerEulBodyForceFunction(
        SAMRAI::tbox::Pointer<IBTK::CartGridFunction> eul_body_force_fcn);

    /*!
     * Register a VisIt data writer so this class will write plot files that may
     * be postprocessed with the VisIt visualization tool.
     */
    void
    registerVisItDataWriter(
        SAMRAI::tbox::Pointer<SAMRAI::appu::VisItDataWriter<NDIM> > visit_writer);

    /*!
     * Register a load balancer for non-uniform load balancing.
     */
    void
    registerLoadBalancer(
        SAMRAI::tbox::Pointer<SAMRAI::mesh::LoadBalancer<NDIM> > load_balancer);

    ///
    ///  The following routines:
    ///
    ///      initializeHierarchyIntegrator(),
    ///      initializeHierarchy(),
    ///      advanceHierarchy(),
    ///      atRegridPoint(),
    ///      getIntegratorTime(),
    ///      getStartTime(),
    ///      getEndTime(),
    ///      getIntegratorStep(),
    ///      getMaxIntegratorSteps(),
    ///      stepsRemaining(),
    ///      getPatchHierarchy(),
    ///      getGriddingAlgorithm(),
    ///      getLDataManager(),
    ///      getIBInstrumentPanel()
    ///
    ///  allow the IBFEHierarchyIntegrator to be used as a hierarchy integrator.
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
        double dt);

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
    SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> >
    getPatchHierarchy() const;

    /*!
     * Return a pointer to the gridding algorithm object.
     */
    SAMRAI::tbox::Pointer<SAMRAI::mesh::GriddingAlgorithm<NDIM> >
    getGriddingAlgorithm() const;

    ///
    ///  The following routines:
    ///
    ///      regridHierarchy(),
    ///      synchronizeHierarchy(),
    ///      synchronizeNewLevels(),
    ///      resetTimeDependentHierData(),
    ///      resetHierDataToPreadvanceState()
    ///
    ///  allow the IBFEHierarchyIntegrator to provide data management for a time
    ///  integrator which making use of this class.
    ///

    /*!
     * Regrid the hierarchy.
     */
    void
    regridHierarchy();

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
        SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        int coarsest_level,
        int finest_level,
        double sync_time,
        bool initial_time);

    /*!
     * Reset time dependent data.
     */
    void
    resetTimeDependentHierData(
        double new_time);

    /*!
     * Deallocate all new simulation data.
     */
    void
    resetHierDataToPreadvanceState();

    ///
    ///  The following routines:
    ///
    ///      getLMarkerSetVar()
    ///
    ///  allows access to the various state variables maintained by the
    ///  integrator.
    ///

    /*!
     * Return a pointer to the LagMarker index data state variable.
     */
    SAMRAI::tbox::Pointer<IBTK::LMarkerSetVariable>
    getLMarkerSetVar() const;

    ///
    ///  The following routines:
    ///
    ///      initializeLevelData(),
    ///      resetHierarchyConfiguration(),
    ///      applyGradientDetector()
    ///
    ///  are concrete implementations of functions declared in the
    ///  SAMRAI::mesh::StandardTagAndInitStrategy<NDIM> abstract base class.
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
        SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
        int level_number,
        double init_data_time,
        bool can_be_refined,
        bool initial_time,
        SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchLevel<NDIM> > old_level=SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchLevel<NDIM> >(NULL),
        bool allocate_data=true);

    /*!
     * Reset cached communication schedules after the hierarchy has changed (for
     * example, due to regidding) and the data has been initialized on the new
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
        SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
        int coarsest_level,
        int finest_level);

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
        SAMRAI::tbox::Pointer<SAMRAI::hier::BasePatchHierarchy<NDIM> > hierarchy,
        int level_number,
        double error_data_time,
        int tag_index,
        bool initial_time,
        bool uses_richardson_extrapolation_too);

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
    IBFEHierarchyIntegrator();

    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBFEHierarchyIntegrator(
        const IBFEHierarchyIntegrator& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBFEHierarchyIntegrator&
    operator=(
        const IBFEHierarchyIntegrator& that);

    /*
     * Common class constructor.
     */
    void
    commonConstructor(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db);

    /*
     * \brief Compute the projected dilatational strain J_bar.
     */
    void
    computeProjectedDilatationalStrain(
        libMesh::NumericVector<double>& J_bar_vec,
        libMesh::NumericVector<double>& X_vec,
        unsigned int part);

    /*
     * \brief Compute the interior elastic density, possibly splitting off the
     * normal component of the transmission force along the physical boundary of
     * the Lagrangian structure.
     */
    void
    computeInteriorForceDensity(
        libMesh::NumericVector<double>& G_vec,
        libMesh::NumericVector<double>& X_vec,
        libMesh::NumericVector<double>* J_bar_vec,
        double time,
        unsigned int part);

    /*!
     * \brief Spread the transmission force density along the physical boundary
     * of the Lagrangian structure.
     */
    void
    spreadTransmissionForceDensity(
        int f_data_idx,
        libMesh::NumericVector<double>& X_ghost_vec,
        libMesh::NumericVector<double>* J_bar_ghost_vec,
        double time,
        unsigned int part);

    /*!
     * \brief Impose jump conditions determined from the interior and
     * transmission force densities along the physical boundary of the
     * Lagrangian structure.
     */
    void
    imposeJumpConditions(
        int f_data_idx,
        libMesh::NumericVector<double>& F_ghost_vec,
        libMesh::NumericVector<double>& X_ghost_vec,
        libMesh::NumericVector<double>* J_bar_ghost_vec,
        double time,
        unsigned int part);

    /*!
     * \brief Initialize the physical coordinates using the supplied coordinate
     * mapping function.  If no function is provided, the initial coordinates
     * are taken to be the Lagrangian coordinates.
     */
    void
    initializeCoordinates(
        unsigned int part);

    /*!
     * \brief Compute dX = X - s, useful mainly for visualization purposes.
     */
    void
    updateCoordinateMapping(
        unsigned int part);

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
     * Pointer to the FE data associated with this time integration object.
     */
    const unsigned int d_num_parts;
    std::vector<IBTK::FEDataManager*> d_fe_data_managers;
    bool d_use_IB_spreading_operator;
    bool d_use_IB_interpolation_operator;
    bool d_split_interior_and_bdry_forces;
    bool d_use_jump_conditions;
    bool d_use_consistent_mass_matrix;
    bool d_use_Fbar_projection;
    libMeshEnums::FEFamily d_fe_family;
    libMeshEnums::Order d_fe_order;
    libMeshEnums::FEFamily d_J_bar_fe_family;
    libMeshEnums::Order d_J_bar_fe_order;
    libMeshEnums::QuadratureType d_quad_type;
    libMeshEnums::Order d_quad_order;

    /*
     * Function used to compute the initial coordinates of the Lagrangian mesh.
     */
    std::vector<CoordinateMappingFcnPtr> d_coordinate_mapping_fcns;
    std::vector<void*> d_coordinate_mapping_fcn_ctxs;

    /*
     * Function used to compute the first Piola-Kirchhoff stress tensor.
     */
    std::vector<PK1StressFcnPtr> d_PK1_stress_fcns;
    std::vector<std::vector<unsigned int> > d_PK1_stress_fcn_systems;
    std::vector<void*> d_PK1_stress_fcn_ctxs;

    /*
     * Optional function use to compute additional body and surface forces on
     * the Lagrangian mesh.
     */
    std::vector<LagBodyForceFcnPtr> d_lag_body_force_fcns;
    std::vector<std::vector<unsigned int> > d_lag_body_force_fcn_systems;
    std::vector<void*> d_lag_body_force_fcn_ctxs;

    std::vector<LagPressureFcnPtr> d_lag_pressure_fcns;
    std::vector<std::vector<unsigned int> > d_lag_pressure_fcn_systems;
    std::vector<void*> d_lag_pressure_fcn_ctxs;

    std::vector<LagSurfaceForceFcnPtr> d_lag_surface_force_fcns;
    std::vector<std::vector<unsigned int> > d_lag_surface_force_fcn_systems;
    std::vector<void*> d_lag_surface_force_fcn_ctxs;

    /*
     * Support for traditional fiber-based Lagrangian data.
     */
    IBTK::LDataManager* d_l_data_manager;
    SAMRAI::tbox::Pointer<IBLagrangianForceStrategy> d_ib_lag_force_strategy;
    bool d_ib_lag_force_strategy_needs_init;

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
     * We cache a pointer to the visualization data writers to register plot
     * variables.
     */
    SAMRAI::tbox::Pointer<SAMRAI::appu::VisItDataWriter<NDIM> > d_visit_writer;

    /*
     * We cache a pointer to the load balancer.
     */
    SAMRAI::tbox::Pointer<SAMRAI::mesh::LoadBalancer<NDIM> > d_load_balancer;

    /*
     * The INSStaggeredHierarchyIntegrator is used to provide time integration
     * capabilitiy for the incompressible Navier-Stokes equations.
     */
    SAMRAI::tbox::Pointer<INSStaggeredHierarchyIntegrator> d_ins_hier_integrator;

    /*
     * An externally-specified body force generator and a force specification
     * function to be passed to the incompressible flow solver.
     */
    SAMRAI::tbox::Pointer<IBTK::CartGridFunction> d_eul_body_force_fcn;
    SAMRAI::tbox::Pointer<IBEulerianForceFunction> d_eulerian_force_fcn;

    /*
     * Integrator data read from input or set at initialization.
     */
    double d_start_time;
    double d_end_time;
    double d_grow_dt;
    int d_max_integrator_steps;

    /*
     * The number of cycles to perform each timestep.
     */
    int d_num_cycles;

    /*
     * The regrid interval indicates the number of integration steps taken
     * between invocations of the regridding process.  The regrid CFL interval
     * indicates the number of meshwidths a particle may move in any coordinate
     * direction between invocations of the regridding process.
     *
     * NOTE: Currently, when the CFL-based regrid interval is specified, it is
     * used instead of the fixed regrid interval.
     */
    int d_regrid_interval;
    double d_regrid_cfl_interval, d_regrid_cfl_estimate;

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
     * Input file for initial marker positions, indices, and clouds.
     */
    std::string d_mark_input_file_name;
    int d_num_mark;
    std::vector<blitz::TinyVector<double,NDIM> > d_mark_init_posns;

    /*
     * Hierarchy operations objects.
     */
    SAMRAI::tbox::Pointer<SAMRAI::math::HierarchySideDataOpsReal<NDIM,double> > d_hier_sc_data_ops;

    /*
     * Communications algorithms and schedules.
     */
    RefineAlgMap           d_ralgs;
    RefinePatchStrategyMap d_rstrategies;
    RefineSchedMap         d_rscheds;

    CoarsenAlgMap           d_calgs;
    CoarsenPatchStrategyMap d_cstrategies;
    CoarsenSchedMap         d_cscheds;

    /*
     * Variables and variable contexts.
     */
    SAMRAI::tbox::Pointer<SAMRAI::pdat::SideVariable<NDIM,double> > d_V_var, d_F_var;
    SAMRAI::tbox::Pointer<IBTK::LMarkerSetVariable> d_mark_var;
    SAMRAI::tbox::Pointer<SAMRAI::hier::VariableContext> d_current, d_scratch;
    int d_V_idx, d_F_idx, d_mark_current_idx, d_mark_scratch_idx;
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBFEHierarchyIntegrator.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBFEHierarchyIntegrator
