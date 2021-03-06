// Copyright (c) 2002-2010, Boyce Griffith, Thomas Fai
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

// Config files
#include <IBTK_config.h>
#include <SAMRAI_config.h>
#include <petsc.h>

// Headers for major SAMRAI objects
#include <BergerRigoutsos.h>
#include <GriddingAlgorithm.h>
#include <HierarchyDataOpsManager.h>
#include <LoadBalancer.h>
#include <StandardTagAndInitialize.h>
#include <VisItDataWriter.h>

// Headers for application-specific algorithm/data structure objects
#include <ibamr/INSStaggeredProjectionPreconditioner.h>
#include <ibamr/INSStaggeredStokesOperator.h>
#include <ibamr/INSStaggeredVCStokesOperator.h>
#include <ibtk/CCLaplaceOperator.h>
#include <ibtk/CCPoissonHypreLevelSolver.h>
#include <ibtk/HierarchyMathOps.h>
#include <ibtk/PETScKrylovLinearSolver.h>
#include <ibtk/PETScSAMRAIVectorReal.h>
#include <ibtk/SCLaplaceOperator.h>
#include <ibtk/SCPoissonHypreLevelSolver.h>
#include <ibtk/muParserCartGridFunction.h>

using namespace SAMRAI;
using namespace IBTK;
using namespace IBAMR;
using namespace std;

/************************************************************************
 *                                                                      *
 * For each run, the input filename must be given on the command line.  *
 * In all cases, the command line is:                                   *
 *                                                                      *
 *    executable <input file name> <PETSc options>                      *
 *                                                                      *
 ************************************************************************
 */

int
main(
    int argc,
    char *argv[])
{
    /*
     * Initialize PETSc, MPI, and SAMRAI.
     */
    PetscInitialize(&argc,&argv,PETSC_NULL,PETSC_NULL);
    tbox::SAMRAI_MPI::setCommunicator(PETSC_COMM_WORLD);
    tbox::SAMRAI_MPI::setCallAbortInSerialInsteadOfExit();
    tbox::SAMRAIManager::startup();

    {// ensure all smart Pointers are properly deleted

        string input_filename = argv[1];
        tbox::plog << "input_filename = " << input_filename << "\n";

        /*
         * Create input database and parse all data in input file.
         */
        tbox::Pointer<tbox::Database> input_db = new tbox::InputDatabase("input_db");
        tbox::InputManager::getManager()->parseInputFile(input_filename, input_db);

        /*
         * Retrieve "Main" section of the input database.
         */
        tbox::Pointer<tbox::Database> main_db = input_db->getDatabase("Main");

        string log_file_name = "laplace_test.log";
        if (main_db->keyExists("log_file_name"))
        {
            log_file_name = main_db->getString("log_file_name");
        }
        bool log_all_nodes = false;
        if (main_db->keyExists("log_all_nodes"))
        {
            log_all_nodes = main_db->getBool("log_all_nodes");
        }
        if (log_all_nodes)
        {
            tbox::PIO::logAllNodes(log_file_name);
        }
        else
        {
            tbox::PIO::logOnlyNodeZero(log_file_name);
        }

        string visit_dump_dirname = "viz";
        if (main_db->keyExists("visit_dump_dirname"))
        {
            visit_dump_dirname = main_db->getString("visit_dump_dirname");
        }

        int visit_number_procs_per_file = 1;
        if (main_db->keyExists("visit_number_procs_per_file"))
        {
            visit_number_procs_per_file = main_db->getInteger("visit_number_procs_per_file");
        }

        bool timer_enabled = false;
        if (main_db->keyExists("timer_enabled"))
        {
            timer_enabled = main_db->getBool("timer_enabled");
        }
        if (timer_enabled)
        {
            tbox::TimerManager::createManager(input_db->getDatabase("TimerManager"));
        }

        /*
         * Create major algorithm and data objects which comprise the
         * application.  Each object will be initialized from input data from
         * the input database.  Refer to each class constructor for details.
         */
        tbox::Pointer<geom::CartesianGridGeometry<NDIM> > grid_geometry =
            new geom::CartesianGridGeometry<NDIM>("CartesianGeometry",
                                                  input_db->getDatabase("CartesianGeometry"));

        tbox::Pointer<hier::PatchHierarchy<NDIM> > patch_hierarchy =
            new hier::PatchHierarchy<NDIM>("PatchHierarchy",
                                           grid_geometry);

        tbox::Pointer<mesh::StandardTagAndInitialize<NDIM> > error_detector =
            new mesh::StandardTagAndInitialize<NDIM>("StandardTagAndInitialize",
                                                     NULL,
                                                     input_db->getDatabase("StandardTagAndInitialize"));

        tbox::Pointer<mesh::BergerRigoutsos<NDIM> > box_generator =
            new mesh::BergerRigoutsos<NDIM>();

        tbox::Pointer<mesh::LoadBalancer<NDIM> > load_balancer =
            new mesh::LoadBalancer<NDIM>("LoadBalancer",
                                         input_db->getDatabase("LoadBalancer"));

        tbox::Pointer<mesh::GriddingAlgorithm<NDIM> > gridding_algorithm =
            new mesh::GriddingAlgorithm<NDIM>("GriddingAlgorithm",
                                              input_db->getDatabase("GriddingAlgorithm"),
                                              error_detector,
                                              box_generator,
                                              load_balancer);

        /*
         * Create variables and register then with the variable database.
         */
        hier::VariableDatabase<NDIM>* var_db = hier::VariableDatabase<NDIM>::getDatabase();
        tbox::Pointer<hier::VariableContext> ctx = var_db->getContext("context");

        tbox::Pointer<pdat::SideVariable<NDIM,double> > u_side_var = new pdat::SideVariable<NDIM,double>("u_side");
        tbox::Pointer<pdat::SideVariable<NDIM,double> > f_side_var = new pdat::SideVariable<NDIM,double>("f_side");
        tbox::Pointer<pdat::SideVariable<NDIM,double> > e_side_var = new pdat::SideVariable<NDIM,double>("e_side");

        const int u_side_idx = var_db->registerVariableAndContext(u_side_var, ctx, hier::IntVector<NDIM>((USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1)));
        const int f_side_idx = var_db->registerVariableAndContext(f_side_var, ctx, hier::IntVector<NDIM>((USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1)));
        const int e_side_idx = var_db->registerVariableAndContext(e_side_var, ctx, hier::IntVector<NDIM>((USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1)));

        tbox::Pointer<pdat::CellVariable<NDIM,double> > u_cell_var = new pdat::CellVariable<NDIM,double>("u_cell",NDIM);
        tbox::Pointer<pdat::CellVariable<NDIM,double> > f_cell_var = new pdat::CellVariable<NDIM,double>("f_cell",NDIM);
        tbox::Pointer<pdat::CellVariable<NDIM,double> > e_cell_var = new pdat::CellVariable<NDIM,double>("e_cell",NDIM);
        tbox::Pointer<pdat::CellVariable<NDIM,double> > p_cell_var = new pdat::CellVariable<NDIM,double>("p_cell");
        tbox::Pointer<pdat::CellVariable<NDIM,double> > du_cell_var = new pdat::CellVariable<NDIM,double>("du_cell");

        const int u_cell_idx = var_db->registerVariableAndContext(u_cell_var, ctx, hier::IntVector<NDIM>(0));
        const int f_cell_idx = var_db->registerVariableAndContext(f_cell_var, ctx, hier::IntVector<NDIM>(0));
        const int e_cell_idx = var_db->registerVariableAndContext(e_cell_var, ctx, hier::IntVector<NDIM>(0));
        const int p_cell_idx = var_db->registerVariableAndContext(p_cell_var, ctx, hier::IntVector<NDIM>((USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1)));
        const int du_cell_idx = var_db->registerVariableAndContext(du_cell_var, ctx, hier::IntVector<NDIM>(0));

        tbox::Pointer<pdat::NodeVariable<NDIM,double> > mu_node_var = new pdat::NodeVariable<NDIM,double>("mu_node");
        tbox::Pointer<pdat::SideVariable<NDIM,double> > rho_side_var = new pdat::SideVariable<NDIM,double>("rho_side");

        const int mu_node_idx = var_db->registerVariableAndContext(mu_node_var, ctx, hier::IntVector<NDIM>((USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1)));
        const int rho_side_idx = var_db->registerVariableAndContext(rho_side_var, ctx, hier::IntVector<NDIM>((USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1)));

        /*
         * Create visualization plot file writer and register variables for
         * plotting.
         */
        tbox::Pointer<appu::VisItDataWriter<NDIM> > visit_data_writer =
            new appu::VisItDataWriter<NDIM>("VisIt Writer",
                                            visit_dump_dirname,
                                            visit_number_procs_per_file);

        visit_data_writer->registerPlotQuantity(u_cell_var->getName(), "VECTOR", u_cell_idx);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            ostringstream stream;
            stream << d;
            visit_data_writer->registerPlotQuantity(u_cell_var->getName()+stream.str(), "SCALAR", u_cell_idx, d);
        }

        visit_data_writer->registerPlotQuantity(f_cell_var->getName(), "VECTOR", f_cell_idx);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            ostringstream stream;
            stream << d;
            visit_data_writer->registerPlotQuantity(f_cell_var->getName()+stream.str(), "SCALAR", f_cell_idx, d);
        }

        visit_data_writer->registerPlotQuantity(e_cell_var->getName(), "VECTOR", e_cell_idx);
        for (unsigned int d = 0; d < NDIM; ++d)
        {
            ostringstream stream;
            stream << d;
            visit_data_writer->registerPlotQuantity(e_cell_var->getName()+stream.str(), "SCALAR", e_cell_idx, d);
        }

        visit_data_writer->registerPlotQuantity(mu_node_var->getName(), "SCALAR", mu_node_idx);

        //visit_data_writer->registerPlotQuantity(rho_side_var->getName(), "SCALAR", rho_side_idx);

        //visit_data_writer->registerPlotQuantity(p_cell_var->getName(), "SCALAR", p_cell_idx);

        /*
         * Initialize the AMR patch hierarchy.
         */
        gridding_algorithm->makeCoarsestLevel(patch_hierarchy, 0.0);

        int tag_buffer = 1;
        int level_number = 0;
        bool done = false;
        while (!done && (gridding_algorithm->levelCanBeRefined(level_number)))
        {
            gridding_algorithm->makeFinerLevel(patch_hierarchy, 0.0, 0.0, tag_buffer);
            done = !patch_hierarchy->finerLevelExists(level_number);
            ++level_number;
        }

        /*
         * Set the simulation time to be zero.
         */
        const double data_time = 0.0;

        /*
         * Allocate data on each level of the patch hierarchy.
         */
        for (int ln = 0; ln <= patch_hierarchy->getFinestLevelNumber(); ++ln)
        {
            tbox::Pointer<hier::PatchLevel<NDIM> > level = patch_hierarchy->getPatchLevel(ln);
            level->allocatePatchData( u_side_idx, data_time);
            level->allocatePatchData( f_side_idx, data_time);
            level->allocatePatchData( e_side_idx, data_time);
            level->allocatePatchData( u_cell_idx, data_time);
            level->allocatePatchData( f_cell_idx, data_time);
            level->allocatePatchData( e_cell_idx, data_time);
            level->allocatePatchData(mu_node_idx, data_time);
            level->allocatePatchData(rho_side_idx, data_time);
            level->allocatePatchData(p_cell_idx, data_time);
            level->allocatePatchData(du_cell_idx, data_time);
	}

        /*
         * Setup exact solution data.
         */
        muParserCartGridFunction u_fcn("u", input_db->getDatabase("u"), grid_geometry);
        muParserCartGridFunction f_fcn("f", input_db->getDatabase("f"), grid_geometry);
        muParserCartGridFunction mu_fcn("mu", input_db->getDatabase("mu"), grid_geometry);
        muParserCartGridFunction rho_fcn("rho", input_db->getDatabase("rho"), grid_geometry);
        muParserCartGridFunction p_fcn("p", input_db->getDatabase("p"), grid_geometry);
        muParserCartGridFunction du_fcn("du", input_db->getDatabase("du"), grid_geometry);

        u_fcn.setDataOnPatchHierarchy(e_side_idx, e_side_var, patch_hierarchy, data_time);
        f_fcn.setDataOnPatchHierarchy(f_side_idx, f_side_var, patch_hierarchy, data_time);
        mu_fcn.setDataOnPatchHierarchy(mu_node_idx, mu_node_var, patch_hierarchy, data_time);
        rho_fcn.setDataOnPatchHierarchy(rho_side_idx, rho_side_var, patch_hierarchy, data_time);
        p_fcn.setDataOnPatchHierarchy(p_cell_idx, p_cell_var, patch_hierarchy, data_time);
        du_fcn.setDataOnPatchHierarchy(du_cell_idx, du_cell_var, patch_hierarchy, data_time);

        /*
         * Problem coefficients for preconditioners.
         */
        double current_time = 0.0;
        double new_time = 0.1;
        double dt = new_time-current_time;
        double rho = 1.0;
        double mu = 1.0;
        double lambda = 0.0;

        /*
         * Create the math operations object and get the patch data index for
         * the side-centered weighting factor.
         */
        HierarchyMathOps hier_math_ops("hier_math_ops", patch_hierarchy);
        const int dx_side_idx = hier_math_ops.getSideWeightPatchDescriptorIndex();
        const int dx_cell_idx = hier_math_ops.getCellWeightPatchDescriptorIndex();

        /*
         * Create the linear solver and solver vectors.
         */
        tbox::Pointer<PETScKrylovLinearSolver> petsc_krylov_solver = new PETScKrylovLinearSolver("petsc_krylov_solver");

        solv::SAMRAIVectorReal<NDIM,double> x("x", patch_hierarchy, 0, patch_hierarchy->getFinestLevelNumber());
	solv::SAMRAIVectorReal<NDIM,double> y("y", patch_hierarchy, 0, patch_hierarchy->getFinestLevelNumber());
        x.addComponent(u_side_var,u_side_idx,dx_side_idx);
	x.addComponent(p_cell_var,p_cell_idx,dx_cell_idx);
	y.addComponent(f_side_var,f_side_idx,dx_side_idx);
        y.addComponent(du_cell_var,du_cell_idx,dx_cell_idx);

        /*
         * Setup the operator.
         */
#if 1
        tbox::Pointer<INSStaggeredVCStokesOperator> vc_stokes_op = new INSStaggeredVCStokesOperator(tbox::Pointer<HierarchyMathOps>(&hier_math_ops,false));
        vc_stokes_op->setTimeInterval(current_time, new_time);
        vc_stokes_op->registerViscosityVariable(mu_node_var,mu_node_idx);
        vc_stokes_op->registerDensityVariable(rho_side_var,rho_side_idx);
        petsc_krylov_solver->setOperator(vc_stokes_op);
#else
        INSCoefs stokes_coefs(rho, mu, lambda);
        blitz::TinyVector<solv::RobinBcCoefStrategy<NDIM>*,NDIM> stokes_U_bc_coefs(NULL);
        tbox::Pointer<INSStaggeredPhysicalBoundaryHelper> stokes_U_bc_helper = NULL;
        solv::RobinBcCoefStrategy<NDIM>* stokes_P_bc_coef = NULL;
        tbox::Pointer<INSStaggeredStokesOperator> stokes_op = new INSStaggeredStokesOperator(stokes_coefs, stokes_U_bc_coefs, stokes_U_bc_helper, stokes_P_bc_coef, tbox::Pointer<HierarchyMathOps>(&hier_math_ops,false));
        stokes_op->setTimeInterval(current_time, new_time);
        petsc_krylov_solver->setOperator(stokes_op);
#endif

        /*
         * Add a nullspace vector to the KSP solver.
         */
        tbox::Pointer<solv::SAMRAIVectorReal<NDIM,double> > nul_vec = x.cloneVector("nul_vec");
        nul_vec->allocateVectorData(0.0);
        tbox::Pointer<math::HierarchyDataOpsReal<NDIM,double> > hier_side_data_ops =
            math::HierarchyDataOpsManager<NDIM>::getManager()->getOperationsDouble(
                u_side_var, patch_hierarchy, true);
        tbox::Pointer<math::HierarchyDataOpsReal<NDIM,double> > hier_cell_data_ops =
            math::HierarchyDataOpsManager<NDIM>::getManager()->getOperationsDouble(
                p_cell_var, patch_hierarchy, true);
        hier_side_data_ops->setToScalar(nul_vec->getComponentDescriptorIndex(0), 0.0);
        hier_cell_data_ops->setToScalar(nul_vec->getComponentDescriptorIndex(1), 1.0);
        petsc_krylov_solver->setNullspace(false, nul_vec);

        /*
         * Setup the velocity subdomain solver.
         */
        const string helmholtz_prefix = "helmholtz_";

        // Setup the various solver components.
        solv::PoissonSpecifications helmholtz_spec("helmholtz_spec");
        helmholtz_spec.setCConstant((rho/dt)+0.5*lambda);
        helmholtz_spec.setDConstant(        -0.5*mu    );

        blitz::TinyVector<solv::RobinBcCoefStrategy<NDIM>*,NDIM> U_star_bc_coefs;
        tbox::Pointer<SCLaplaceOperator> helmholtz_op = new SCLaplaceOperator("Helmholtz Operator", helmholtz_spec, U_star_bc_coefs, true);
        helmholtz_op->setHierarchyMathOps(tbox::Pointer<HierarchyMathOps>(&hier_math_ops,false));

        tbox::Pointer <PETScKrylovLinearSolver> helmholtz_solver = new PETScKrylovLinearSolver("::Helmholtz Krylov Solver", helmholtz_prefix);
        helmholtz_solver->setInitialGuessNonzero(false);
        helmholtz_solver->setOperator(helmholtz_op);

        tbox::Pointer<SCPoissonHypreLevelSolver> helmholtz_hypre_pc = new SCPoissonHypreLevelSolver("Helmholtz Preconditioner", input_db->getDatabase("HelmholtzHypreSolver"));
        helmholtz_hypre_pc->setPoissonSpecifications(helmholtz_spec);
        helmholtz_solver->setPreconditioner(helmholtz_hypre_pc);

        // Set some default options.
        helmholtz_solver->setKSPType("preonly");
        helmholtz_solver->setAbsoluteTolerance(1.0e-30);
        helmholtz_solver->setRelativeTolerance(1.0e-02);
        helmholtz_solver->setMaxIterations(25);

        /*
         * Setup the pressure subdomain solver.
         */
        const string poisson_prefix = "poisson_";

        // Setup the various solver components.
        solv::PoissonSpecifications poisson_spec("poisson_spec");
        poisson_spec.setCZero();
        poisson_spec.setDConstant(-1.0);

        solv::RobinBcCoefStrategy<NDIM>* Phi_bc_coef = NULL;
        tbox::Pointer<CCLaplaceOperator> poisson_op = new CCLaplaceOperator("Poisson Operator", poisson_spec, Phi_bc_coef, true);
        poisson_op->setHierarchyMathOps(tbox::Pointer<HierarchyMathOps>(&hier_math_ops,false));

        tbox::Pointer<PETScKrylovLinearSolver> poisson_solver = new PETScKrylovLinearSolver("Poisson Krylov Solver", poisson_prefix);
        poisson_solver->setInitialGuessNonzero(false);
        poisson_solver->setOperator(poisson_op);

        tbox::Pointer<CCPoissonHypreLevelSolver> poisson_hypre_pc = new CCPoissonHypreLevelSolver("Poisson Preconditioner", input_db->getDatabase("PoissonHypreSolver"));
        poisson_hypre_pc->setPoissonSpecifications(poisson_spec);
        poisson_solver->setPreconditioner(poisson_hypre_pc);

        // Set some default options.
        poisson_solver->setKSPType("preonly");
        poisson_solver->setAbsoluteTolerance(1.0e-30);
        poisson_solver->setRelativeTolerance(1.0e-02);
        poisson_solver->setMaxIterations(25);
        poisson_solver->setNullspace(true, NULL);

        /*
         * Setup the Stokes preconditioner.
         */
        INSCoefs problem_coefs(rho, mu, lambda);
        tbox::Pointer<INSStaggeredProjectionPreconditioner> projection_pc = new INSStaggeredProjectionPreconditioner(problem_coefs, Phi_bc_coef, true, helmholtz_solver, poisson_solver, hier_cell_data_ops, hier_side_data_ops, tbox::Pointer<HierarchyMathOps>(&hier_math_ops,false));
        projection_pc->setTimeInterval(current_time, new_time, dt);
        petsc_krylov_solver->setPreconditioner(projection_pc);

        /*
         * Initialize the solver.
         */
        tbox::TimerManager* timer_manager = tbox::TimerManager::getManager();
        tbox::Pointer<tbox::Timer> t_initialize_solver_state = timer_manager->getTimer("IBTK::main::initializeSolverState()");
        tbox::Pointer<tbox::Timer> t_solve_system = timer_manager->getTimer("IBTK::main::solveSystem()");

        t_initialize_solver_state->start();
        petsc_krylov_solver->initializeSolverState(x,y);
        t_initialize_solver_state->stop();

        /*
         * Solve the system.
         */
        x.setToScalar(0.0,false);
        t_solve_system->start();
        petsc_krylov_solver->solveSystem(x,y);
        t_solve_system->stop();

        /*
         * Compute error and print error norms.
         */
        hier_side_data_ops->subtract(e_side_idx, e_side_idx, u_side_idx);  // computes e := e - u

        tbox::pout << "|e|_oo = " << hier_side_data_ops->maxNorm(e_side_idx, dx_side_idx) << "\n";
        tbox::pout << "|e|_2  = " << hier_side_data_ops-> L2Norm(e_side_idx, dx_side_idx) << "\n";
        tbox::pout << "|e|_1  = " << hier_side_data_ops-> L1Norm(e_side_idx, dx_side_idx) << "\n";

        /*
         * Interpolate the side-centered data to cell centers (for output
         * purposes only).
         */
        static const bool synch_cf_interface = true;
        hier_math_ops.interp(u_cell_idx, u_cell_var, u_side_idx, u_side_var, NULL, data_time, synch_cf_interface);
        hier_math_ops.interp(f_cell_idx, f_cell_var, f_side_idx, f_side_var, NULL, data_time, synch_cf_interface);
        hier_math_ops.interp(e_cell_idx, e_cell_var, e_side_idx, e_side_var, NULL, data_time, synch_cf_interface);

        /*
         * Output data for plotting.
         */
        visit_data_writer->writePlotData(patch_hierarchy, 0, data_time);

    }// ensure all smart Pointers are properly deleted

    tbox::SAMRAIManager::shutdown();
    PetscFinalize();

    return 0;
}// main
