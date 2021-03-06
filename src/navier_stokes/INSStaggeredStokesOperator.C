// Filename: INSStaggeredStokesOperator.C
// Created on 29 Apr 2008 by Boyce Griffith
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

#include "INSStaggeredStokesOperator.h"

/////////////////////////////// INCLUDES /////////////////////////////////////

#ifndef included_IBAMR_config
#include <IBAMR_config.h>
#define included_IBAMR_config
#endif

#ifndef included_SAMRAI_config
#include <SAMRAI_config.h>
#define included_SAMRAI_config
#endif

// IBAMR INCLUDES
#include <ibamr/INSStaggeredPressureBcCoef.h>
#include <ibamr/ibamr_utilities.h>
#include <ibamr/namespaces.h>

// IBTK INCLUDES
#include <ibtk/CellNoCornersFillPattern.h>
#include <ibtk/SideNoCornersFillPattern.h>

// C++ STDLIB INCLUDES
#include <limits>

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBAMR
{
/////////////////////////////// STATIC ///////////////////////////////////////

namespace
{
// Number of ghosts cells used for each variable quantity.
static const int CELLG = (USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1);
static const int SIDEG = (USING_LARGE_GHOST_CELL_WIDTH ? 2 : 1);

// Type of coarsening to perform prior to setting coarse-fine boundary and
// physical boundary ghost cell values.
static const std::string DATA_COARSEN_TYPE = "CUBIC_COARSEN";

// Type of extrapolation to use at physical boundaries.
static const std::string BDRY_EXTRAP_TYPE = "LINEAR";

// Whether to enforce consistent interpolated values at Type 2 coarse-fine
// interface ghost cells.
static const bool CONSISTENT_TYPE_2_BDRY = false;

// Timers.
static Timer* t_apply;
static Timer* t_initialize_operator_state;
static Timer* t_deallocate_operator_state;
}

/////////////////////////////// PUBLIC ///////////////////////////////////////

INSStaggeredStokesOperator::INSStaggeredStokesOperator(
    const INSProblemCoefs* problem_coefs,
    blitz::TinyVector<RobinBcCoefStrategy<NDIM>*,NDIM> U_bc_coefs,
    RobinBcCoefStrategy<NDIM>* P_bc_coef,
    Pointer<HierarchyMathOps> hier_math_ops)
    : d_is_initialized(false),
      d_current_time(std::numeric_limits<double>::quiet_NaN()),
      d_new_time(std::numeric_limits<double>::quiet_NaN()),
      d_U_fill_pattern(NULL),
      d_P_fill_pattern(NULL),
      d_transaction_comps(),
      d_hier_bdry_fill(Pointer<HierarchyGhostCellInterpolation>(NULL)),
      d_no_fill(Pointer<HierarchyGhostCellInterpolation>(NULL)),
      d_x(NULL),
      d_b(NULL),
      d_problem_coefs(problem_coefs),
      d_helmholtz_spec("INSStaggeredStokesOperator::helmholtz_spec"),
      d_U_bc_coefs(U_bc_coefs),
      d_P_bc_coef(P_bc_coef),
      d_homogeneous_bc(false),
      d_correcting_rhs(false),
      d_hier_math_ops(hier_math_ops)
{
    // Setup Timers.
    IBAMR_DO_ONCE(
        t_apply                     = TimerManager::getManager()->getTimer("IBAMR::INSStaggeredStokesOperator::apply()");
        t_initialize_operator_state = TimerManager::getManager()->getTimer("IBAMR::INSStaggeredStokesOperator::initializeOperatorState()");
        t_deallocate_operator_state = TimerManager::getManager()->getTimer("IBAMR::INSStaggeredStokesOperator::deallocateOperatorState()");
                  );
    return;
}// INSStaggeredStokesOperator

INSStaggeredStokesOperator::~INSStaggeredStokesOperator()
{
    deallocateOperatorState();
    return;
}// ~INSStaggeredStokesOperator

void
INSStaggeredStokesOperator::setHomogeneousBc(
    const bool homogeneous_bc)
{
    d_homogeneous_bc = homogeneous_bc;
    return;
}// setHomogeneousBc

void
INSStaggeredStokesOperator::setTimeInterval(
    const double current_time,
    const double new_time)
{
    const double rho    = d_problem_coefs->getRho();
    const double mu     = d_problem_coefs->getMu();
    const double lambda = d_problem_coefs->getLambda();
    d_current_time = current_time;
    d_new_time = new_time;
    const double dt = d_new_time-d_current_time;
    d_helmholtz_spec.setCConstant((rho/dt)+0.5*lambda);
    d_helmholtz_spec.setDConstant(        -0.5*mu    );
    return;
}// setTimeInterval

void
INSStaggeredStokesOperator::modifyRhsForInhomogeneousBc(
    SAMRAIVectorReal<NDIM,double>& y)
{
    // Set y := y - A*0, i.e., shift the right-hand-side vector to account for
    // inhomogeneous boundary conditions.
    if (!d_homogeneous_bc)
    {
        d_correcting_rhs = true;
        apply(*d_x,*d_b);
        y.subtract(Pointer<SAMRAIVectorReal<NDIM,double> >(&y, false), d_b);
        d_correcting_rhs = false;
    }
    return;
}// modifyRhsForInhomogeneousBc

void
INSStaggeredStokesOperator::apply(
    const bool homogeneous_bc,
    SAMRAIVectorReal<NDIM,double>& x,
    SAMRAIVectorReal<NDIM,double>& y)
{
    IBAMR_TIMER_START(t_apply);

    // Get the vector components.
    const int U_in_idx  = x.getComponentDescriptorIndex(0);
    const int P_in_idx  = x.getComponentDescriptorIndex(1);
    const int U_out_idx = y.getComponentDescriptorIndex(0);
    const int P_out_idx = y.getComponentDescriptorIndex(1);

    Pointer<SideVariable<NDIM,double> > U_in_sc_var  = x.getComponentVariable(0);
    Pointer<CellVariable<NDIM,double> > P_in_cc_var  = x.getComponentVariable(1);
    Pointer<SideVariable<NDIM,double> > U_out_sc_var = y.getComponentVariable(0);
    Pointer<CellVariable<NDIM,double> > P_out_cc_var = y.getComponentVariable(1);

    // Simultaneously fill ghost cell values for all components.
    typedef HierarchyGhostCellInterpolation::InterpolationTransactionComponent InterpolationTransactionComponent;
    std::vector<InterpolationTransactionComponent> x_components(2);
    x_components[0] = InterpolationTransactionComponent(U_in_idx, DATA_COARSEN_TYPE, BDRY_EXTRAP_TYPE, CONSISTENT_TYPE_2_BDRY, d_U_bc_coefs, d_U_fill_pattern);
    x_components[1] = InterpolationTransactionComponent(P_in_idx, DATA_COARSEN_TYPE, BDRY_EXTRAP_TYPE, CONSISTENT_TYPE_2_BDRY, d_P_bc_coef , d_P_fill_pattern);
    INSStaggeredPressureBcCoef* P_bc_coef = dynamic_cast<INSStaggeredPressureBcCoef*>(d_P_bc_coef);
    if (P_bc_coef != NULL) P_bc_coef->setVelocityNewPatchDataIndex(U_in_idx);
    d_hier_bdry_fill->setHomogeneousBc(homogeneous_bc);
    d_hier_bdry_fill->resetTransactionComponents(x_components);
    d_hier_bdry_fill->fillData(d_new_time);
    d_hier_bdry_fill->resetTransactionComponents(d_transaction_comps);

    // Compute the action of the operator:
    //      A*[u;p] = [((rho/dt)*I-0.5*mu*L)*u + grad p; -div u].
    d_hier_math_ops->grad(U_out_idx, U_out_sc_var, /*cf_bdry_synch*/ false, 1.0, P_in_idx, P_in_cc_var, d_no_fill, d_new_time);
    d_hier_math_ops->laplace(U_out_idx, U_out_sc_var, d_helmholtz_spec, U_in_idx, U_in_sc_var, d_no_fill, d_new_time, 1.0, U_out_idx, U_out_sc_var);
    d_hier_math_ops->div(P_out_idx, P_out_cc_var, -1.0, U_in_idx, U_in_sc_var, d_no_fill, d_new_time, /*cf_bdry_synch*/ true);

    IBAMR_TIMER_STOP(t_apply);
    return;
}// apply

void
INSStaggeredStokesOperator::apply(
    SAMRAIVectorReal<NDIM,double>& x,
    SAMRAIVectorReal<NDIM,double>& y)
{
    apply(d_correcting_rhs ? d_homogeneous_bc : true, x, y);
    return;
}// apply

void
INSStaggeredStokesOperator::initializeOperatorState(
    const SAMRAIVectorReal<NDIM,double>& in,
    const SAMRAIVectorReal<NDIM,double>& out)
{
    IBAMR_TIMER_START(t_initialize_operator_state);

    // Deallocate the operator state if the operator is already initialized.
    if (d_is_initialized) deallocateOperatorState();

    // Setup solution and rhs vectors.
    d_x = in .cloneVector(in .getName());
    d_b = out.cloneVector(out.getName());

    d_x->allocateVectorData();
    d_x->setToScalar(0.0);
    d_b->allocateVectorData();

    // Setup the interpolation transaction information.
    d_U_fill_pattern = new SideNoCornersFillPattern(SIDEG, false, false, true);
    d_P_fill_pattern = new CellNoCornersFillPattern(CELLG, false, false, true);
    typedef HierarchyGhostCellInterpolation::InterpolationTransactionComponent InterpolationTransactionComponent;
    d_transaction_comps.resize(2);
    d_transaction_comps[0] = InterpolationTransactionComponent(d_x->getComponentDescriptorIndex(0), DATA_COARSEN_TYPE, BDRY_EXTRAP_TYPE, CONSISTENT_TYPE_2_BDRY, d_U_bc_coefs, d_U_fill_pattern);
    d_transaction_comps[1] = InterpolationTransactionComponent(d_x->getComponentDescriptorIndex(1), DATA_COARSEN_TYPE, BDRY_EXTRAP_TYPE, CONSISTENT_TYPE_2_BDRY, d_P_bc_coef , d_P_fill_pattern);

    // Initialize the interpolation operators.
    d_hier_bdry_fill = new HierarchyGhostCellInterpolation();
    d_hier_bdry_fill->initializeOperatorState(d_transaction_comps, d_x->getPatchHierarchy());

    // Indicate the operator is initialized.
    d_is_initialized = true;

    IBAMR_TIMER_STOP(t_initialize_operator_state);
    return;
}// initializeOperatorState

void
INSStaggeredStokesOperator::deallocateOperatorState()
{
    if (!d_is_initialized) return;

    IBAMR_TIMER_START(t_deallocate_operator_state);

    // Deallocate the interpolation operators.
    d_hier_bdry_fill->deallocateOperatorState();
    d_hier_bdry_fill.setNull();
    d_transaction_comps.clear();
    d_U_fill_pattern.setNull();
    d_P_fill_pattern.setNull();

    // Delete the solution and rhs vectors.
    d_x->resetLevels(d_x->getCoarsestLevelNumber(), std::min(d_x->getFinestLevelNumber(),d_x->getPatchHierarchy()->getFinestLevelNumber()));
    d_x->freeVectorComponents();

    d_b->resetLevels(d_b->getCoarsestLevelNumber(), std::min(d_b->getFinestLevelNumber(),d_b->getPatchHierarchy()->getFinestLevelNumber()));
    d_b->freeVectorComponents();

    d_x.setNull();
    d_b.setNull();

    // Indicate that the operator is NOT initialized.
    d_is_initialized = false;

    IBAMR_TIMER_STOP(t_deallocate_operator_state);
    return;
}// deallocateOperatorState

void
INSStaggeredStokesOperator::enableLogging(
    bool /*enabled*/)
{
    // intentionally blank
    return;
}// enableLogging

/////////////////////////////// PROTECTED ////////////////////////////////////

/////////////////////////////// PRIVATE //////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////

}// namespace IBAMR

//////////////////////////////////////////////////////////////////////////////
