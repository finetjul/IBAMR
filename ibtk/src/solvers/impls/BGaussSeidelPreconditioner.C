// Filename: BGaussSeidelPreconditioner.C
// Created on 15 Sep 2006 by Boyce Griffith
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

#include "BGaussSeidelPreconditioner.h"

/////////////////////////////// INCLUDES /////////////////////////////////////

#ifndef included_IBTK_config
#include <IBTK_config.h>
#define included_IBTK_config
#endif

#ifndef included_SAMRAI_config
#include <SAMRAI_config.h>
#define included_SAMRAI_config
#endif

// IBTK INCLUDES
#include <ibtk/namespaces.h>

// C++ STDLIB INCLUDES
#include <ostream>

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBTK
{
/////////////////////////////// STATIC ///////////////////////////////////////

/////////////////////////////// PUBLIC ///////////////////////////////////////

BGaussSeidelPreconditioner::BGaussSeidelPreconditioner(
    Pointer<Database> input_db)
    : d_is_initialized(false),
      d_pc_map(),
      d_linear_ops_map(),
      d_symmetric_preconditioner(false),
      d_reverse_order(false),
      d_initial_guess_nonzero(false),
      d_rel_residual_tol(1.0e-6),
      d_abs_residual_tol(1.0e-30),
      d_max_iterations(1)
{
    // Get configuration data from the input database.
    if (!input_db.isNull())
    {
        // Block Gauss-Seidel options.
        d_symmetric_preconditioner = input_db->getBoolWithDefault(
            "symmetric_preconditioner", d_symmetric_preconditioner);
        d_reverse_order = input_db->getBoolWithDefault(
            "reverse_order", d_reverse_order);

        // LinearSolver options.
        d_initial_guess_nonzero = input_db->getBoolWithDefault(
            "initial_guess_nonzero", d_initial_guess_nonzero);
        d_rel_residual_tol = input_db->getDoubleWithDefault(
            "rel_residual_tol", d_rel_residual_tol);
        d_abs_residual_tol = input_db->getDoubleWithDefault(
            "abs_residual_tol", d_abs_residual_tol);
        d_max_iterations = input_db->getIntegerWithDefault(
            "max_iterations", d_max_iterations);
    }
    return;
}// BGaussSeidelPreconditioner()

BGaussSeidelPreconditioner::~BGaussSeidelPreconditioner()
{
    if (d_is_initialized) deallocateSolverState();
    return;
}// ~BGaussSeidelPreconditioner()

void
BGaussSeidelPreconditioner::setComponentPreconditioner(
    Pointer<LinearSolver> preconditioner,
    const unsigned int component)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(!preconditioner.isNull());
#endif
    d_pc_map[component] = preconditioner;
    return;
}// setComponentPreconditioner

void
BGaussSeidelPreconditioner::setComponentOperators(
    const std::vector<Pointer<LinearOperator> >& linear_ops,
    const unsigned int component)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    for (unsigned int k = 0; k < linear_ops.size(); ++k)
    {
        if (k != component) TBOX_ASSERT(!linear_ops[k].isNull());
    }
#endif
    d_linear_ops_map[component] = linear_ops;
    return;
}// setComponentOperators

void
BGaussSeidelPreconditioner::setSymmetricPreconditioner(
    const bool symmetric_preconditioner)
{
    d_symmetric_preconditioner = symmetric_preconditioner;
    return;
}// setSymmetricPreconditioner

void
BGaussSeidelPreconditioner::setReversedOrder(
    const bool reverse_order)
{
    d_reverse_order = reverse_order;
    return;
}// setReversedOrder

bool
BGaussSeidelPreconditioner::solveSystem(
    SAMRAIVectorReal<NDIM,double>& x,
    SAMRAIVectorReal<NDIM,double>& b)
{
    /*!
     * \todo Setup the solver so that it works correctly for multiple
     * iterations.
     */
    TBOX_ASSERT(d_max_iterations == 1);

    // Initialize the preconditioner, when necessary.
    const bool deallocate_after_solve = !d_is_initialized;
    if (deallocate_after_solve) initializeSolverState(x,b);

#ifdef DEBUG_CHECK_ASSERTIONS
    Pointer<PatchHierarchy<NDIM> > hierarchy = x.getPatchHierarchy();
    const int coarsest_ln = x.getCoarsestLevelNumber();
    const int   finest_ln = x.getFinestLevelNumber()  ;
    TBOX_ASSERT(x.getNumberOfComponents() == b.getNumberOfComponents());
    TBOX_ASSERT(hierarchy == b.getPatchHierarchy());
    TBOX_ASSERT(coarsest_ln == b.getCoarsestLevelNumber());
    TBOX_ASSERT(  finest_ln == b.getFinestLevelNumber()  );
#endif
    bool ret_val = true;

    // Zero out the initial guess.
    if (!d_initial_guess_nonzero) x.setToScalar(0.0);

    // Setup SAMRAIVectorReal objects to correspond to the individual vector
    // components.
    std::vector<Pointer<SAMRAIVectorReal<NDIM,double> > > x_comps =
        getComponentVectors(Pointer<SAMRAIVectorReal<NDIM,double> >(&x,false));
    std::vector<Pointer<SAMRAIVectorReal<NDIM,double> > > b_comps =
        getComponentVectors(Pointer<SAMRAIVectorReal<NDIM,double> >(&b,false));

    // Clone the right-hand-side vector to avoid modifying it during the
    // preconditioning operation.
    Pointer<SAMRAIVectorReal<NDIM,double> > f =
        b.cloneVector(b.getName());
    f->allocateVectorData();
    f->copyVector(
        Pointer<SAMRAIVectorReal<NDIM,double> >(&b,false),
        false);
    std::vector<Pointer<SAMRAIVectorReal<NDIM,double> > > f_comps =
        getComponentVectors(f);

    // Setup the order in which the component preconditioner are to be applied.
    const int ncomps = x.getNumberOfComponents();
    std::vector<int> comps;
    comps.reserve(2*ncomps-1);
    if (!d_reverse_order)
    {
        // Standard order: Run from comp = 0 to comp = ncomp-1.
        for (int comp = 0; comp < ncomps; ++comp)
        {
            comps.push_back(comp);
        }
        if (d_symmetric_preconditioner)
        {
            for (int comp = ncomps - 2; comp >= 0; --comp)
            {
                comps.push_back(comp);
            }
        }
    }
    else
    {
        // Reversed order: Run from comp = ncomp-1 to comp = 0.
        for (int comp = ncomps-1; comp >= 0; --comp)
        {
            comps.push_back(comp);
        }
        if (d_symmetric_preconditioner)
        {
            for (int comp = 1; comp < ncomps; ++comp)
            {
                comps.push_back(comp);
            }
        }
    }

    // Apply the component preconditioners.
    for (std::vector<int>::const_iterator it = comps.begin();
         it != comps.end(); ++it)
    {
        const int comp = (*it);

        Pointer<SAMRAIVectorReal<NDIM,double> > x_comp = x_comps[comp];
        Pointer<SAMRAIVectorReal<NDIM,double> > b_comp = b_comps[comp];
        Pointer<SAMRAIVectorReal<NDIM,double> > f_comp = f_comps[comp];

        // Update the right-hand-side vector.
        f_comp->setToScalar(0.0);
        for (int c = 0; c < ncomps; ++c)
        {
            // Skip the diagonal operators.
            if (c != comp)
            {
                d_linear_ops_map[comp][c]->applyAdd(
                    *x_comps[c], *f_comp, *f_comp);
            }
        }
        f_comp->subtract(b_comp, f_comp);

        // Configure the component preconditioner.
        Pointer<LinearSolver> pc_comp = d_pc_map[comp];
        pc_comp->setInitialGuessNonzero(true);
        pc_comp->setMaxIterations(d_max_iterations);
        pc_comp->setAbsoluteTolerance(d_abs_residual_tol);
        pc_comp->setRelativeTolerance(d_rel_residual_tol);

        // Apply the component preconditioner.
        const bool ret_val_comp = pc_comp->solveSystem(*x_comp, *f_comp);
        ret_val = ret_val && ret_val_comp;
    }

    // Free the copied right-hand-side vector data.
    f->freeVectorComponents();

    // Deallocate the preconditioner, when necessary.
    if (deallocate_after_solve) deallocateSolverState();

    return ret_val;
}// solveSystem

void
BGaussSeidelPreconditioner::initializeSolverState(
    const SAMRAIVectorReal<NDIM,double>& x,
    const SAMRAIVectorReal<NDIM,double>& b)
{
#ifdef DEBUG_CHECK_ASSERTIONS
    Pointer<PatchHierarchy<NDIM> > hierarchy = x.getPatchHierarchy();
    const int coarsest_ln = x.getCoarsestLevelNumber();
    const int finest_ln = x.getFinestLevelNumber();
    TBOX_ASSERT(hierarchy == b.getPatchHierarchy());
    TBOX_ASSERT(coarsest_ln == b.getCoarsestLevelNumber());
    TBOX_ASSERT(finest_ln == b.getFinestLevelNumber());
    TBOX_ASSERT(x.getNumberOfComponents() == b.getNumberOfComponents());
#endif
    // Setup SAMRAIVectorReal objects to correspond to the individual vector
    // components.
    std::vector<Pointer<SAMRAIVectorReal<NDIM,double> > > x_comps =
        getComponentVectors(ConstPointer<SAMRAIVectorReal<NDIM,double> >(&x,false));
    std::vector<Pointer<SAMRAIVectorReal<NDIM,double> > > b_comps =
        getComponentVectors(ConstPointer<SAMRAIVectorReal<NDIM,double> >(&b,false));

    // Initialize the component operators and preconditioners.
    const int ncomps = x.getNumberOfComponents();
    for (int comp = 0; comp < ncomps; ++comp)
    {
        for (int c = 0; c < ncomps; ++c)
        {
            // Skip the diagonal operators.
            if (c != comp)
            {
                d_linear_ops_map[comp][c]->initializeOperatorState(
                    *x_comps[comp], *b_comps[comp]);
            }
        }
        d_pc_map[comp]->initializeSolverState(*x_comps[comp], *b_comps[comp]);
    }

    // Indicate that the preconditioner is initialized.
    d_is_initialized = true;

    return;
}// initializeSolverState

void
BGaussSeidelPreconditioner::deallocateSolverState()
{
    if (!d_is_initialized) return;

    // Deallocate the component preconditioners.
    for (std::map<unsigned int,Pointer<LinearSolver> >::iterator it = d_pc_map.begin();
         it != d_pc_map.end(); ++it)
    {
        it->second->deallocateSolverState();
    }

    // Deallocate the component operators.
    for (std::map<unsigned int,std::vector<Pointer<LinearOperator> > >::iterator it = d_linear_ops_map.begin();
         it != d_linear_ops_map.end(); ++it)
    {
        std::vector<Pointer<LinearOperator> >& comp_linear_ops = it->second;
        for (std::vector<Pointer<LinearOperator> >::iterator comp_it = comp_linear_ops.begin();
             comp_it != comp_linear_ops.end(); ++comp_it)
        {
            if (!comp_it->isNull()) (*comp_it)->deallocateOperatorState();
        }
    }

    // Indicate that the preconditioner is NOT initialized.
    d_is_initialized = false;

    return;
}// deallocateSolverState

/////////////////////////////// PRIVATE //////////////////////////////////////

std::vector<Pointer<SAMRAIVectorReal<NDIM,double> > >
BGaussSeidelPreconditioner::getComponentVectors(
    const ConstPointer<SAMRAIVectorReal<NDIM,double> > x)
{
    Pointer<PatchHierarchy<NDIM> > hierarchy = x->getPatchHierarchy();
    const int coarsest_ln = x->getCoarsestLevelNumber();
    const int finest_ln = x->getFinestLevelNumber();
    const std::string& x_name = x->getName();
    const int ncomps = x->getNumberOfComponents();

    // Setup SAMRAIVectorReal objects to correspond to the individual vector
    // components.
    std::vector<Pointer<SAMRAIVectorReal<NDIM,double> > > x_comps(ncomps);
    for (int comp = 0; comp < ncomps; ++comp)
    {
        std::ostringstream str;
        str << comp;
        x_comps[comp] = new SAMRAIVectorReal<NDIM,double>(
            x_name+"_component_"+str.str(),
            hierarchy, coarsest_ln, finest_ln);
        x_comps[comp]->addComponent(x->getComponentVariable(comp),
                                    x->getComponentDescriptorIndex(comp),
                                    x->getControlVolumeIndex(comp));
    }
    return x_comps;
}// getComponentVectors

//////////////////////////////////////////////////////////////////////////////

}// namespace IBTK

//////////////////////////////////////////////////////////////////////////////
