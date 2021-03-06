// Filename: FACPreconditioner.C
// Created on 25 Aug 2010 by Boyce Griffith
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

#include "FACPreconditioner.h"

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBTK INCLUDES
#include <ibtk/FACPreconditionerStrategy.h>
#include <ibtk/namespaces.h>

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBTK
{
/////////////////////////////// STATIC ///////////////////////////////////////

/////////////////////////////// PUBLIC ///////////////////////////////////////

FACPreconditioner::FACPreconditioner(
    const std::string& object_name,
    Pointer<FACPreconditionerStrategy> fac_strategy,
    tbox::Pointer<tbox::Database> input_db)
    : d_object_name(object_name),
      d_is_initialized(false),
      d_fac_strategy(fac_strategy),
      d_hierarchy(NULL),
      d_coarsest_ln(0),
      d_finest_ln(0),
      d_cycle_type(V_CYCLE),
      d_num_pre_sweeps(1),
      d_num_post_sweeps(1),
      d_f(),
      d_r(),
      d_do_log(false)
{
    /*
     * Register this class with the FACPreconditionerStrategy object.
     */
    d_fac_strategy->setFACPreconditioner(Pointer<FACPreconditioner>(this,false));

    /*
     * Initialize object with data read from input database.
     */
    if (!input_db.isNull())
    {
        getFromInput(input_db);
    }
    return;
}// FACPreconditioner

FACPreconditioner::~FACPreconditioner()
{
    if (d_is_initialized) deallocateSolverState();
    return;
}// ~FACPreconditioner

void
FACPreconditioner::setTimeInterval(
    const double current_time,
    const double new_time)
{
    d_fac_strategy->setTimeInterval(current_time, new_time);
    return;
}// setTimeInterval

bool
FACPreconditioner::solveSystem(
    SAMRAIVectorReal<NDIM,double>& u,
    SAMRAIVectorReal<NDIM,double>& f)
{
    // Initialize the solver, when necessary.
    const bool deallocate_after_solve = !d_is_initialized;
    if (deallocate_after_solve) initializeSolverState(u,f);

    // Keep track of whether we need to (re-)compute the residual.  u is
    // required to be zero upon entry to this function, so as long as u is
    // unmodified, the residual is simply equal to the right-hand-side vector f.
    d_recompute_residual = false;

    // Apply a single FAC cycle.
    if (d_cycle_type == V_CYCLE && d_num_pre_sweeps == 0)
    {
        // V-cycle MG without presmoothing keeps the residual equal to the
        // initial right-hand-side vector f, so we can simply use that vector
        // for the residual in the FAC algorithm.
        FACVCycleNoPreSmoothing(u, f, d_finest_ln);
    }
    else
    {
        d_f->copyVector(Pointer<SAMRAIVectorReal<NDIM,double> >(&f, false), false);
        d_r->copyVector(Pointer<SAMRAIVectorReal<NDIM,double> >(&f, false), false);
        switch (d_cycle_type)
        {
            case V_CYCLE:
                FACVCycle(u, *d_f, d_finest_ln);
                break;
            case W_CYCLE:
                FACWCycle(u, *d_f, d_finest_ln);
                break;
            case F_CYCLE:
                FACFCycle(u, *d_f, d_finest_ln);
                break;
            default:
                TBOX_ERROR(d_object_name << "::solveSystem():\n"
                           << "  unrecognized FAC cycle type: " << enum_to_string<MGCycleType>(d_cycle_type) << "." << std::endl);
        }
    }

    // Deallocate the solver, when necessary.
    if (deallocate_after_solve) deallocateSolverState();
    return true;
}// solveSystem

void
FACPreconditioner::initializeSolverState(
    const SAMRAIVectorReal<NDIM,double>& solution,
    const SAMRAIVectorReal<NDIM,double>& rhs)
{
    // Deallocate the solver state if the solver is already initialized.
    if (d_is_initialized)
    {
        deallocateSolverState();
    }

    // Setup operator state.
    d_hierarchy   = solution.getPatchHierarchy();
    d_coarsest_ln = solution.getCoarsestLevelNumber();
    d_finest_ln   = solution.getFinestLevelNumber();

#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(d_hierarchy   == rhs.getPatchHierarchy());
    TBOX_ASSERT(d_coarsest_ln == rhs.getCoarsestLevelNumber());
    TBOX_ASSERT(d_finest_ln   == rhs.getFinestLevelNumber());
#endif
    d_fac_strategy->initializeOperatorState(solution, rhs);

    // Create temporary vectors.
    if (!(d_cycle_type == V_CYCLE && d_num_pre_sweeps == 0))
    {
        d_f = rhs.cloneVector("");
        d_f->allocateVectorData();

        d_r = rhs.cloneVector("");
        d_r->allocateVectorData();
    }

    // Indicate the operator is initialized.
    d_is_initialized = true;
    return;
}// initializeSolverState

void
FACPreconditioner::deallocateSolverState()
{
    if (!d_is_initialized) return;

    // Destroy temporary vectors.
    if (!d_f.isNull())
    {
        d_f->resetLevels(d_f->getCoarsestLevelNumber(), std::min(d_f->getFinestLevelNumber(),d_f->getPatchHierarchy()->getFinestLevelNumber()));
        d_f->freeVectorComponents();
        d_f.setNull();
    }

    if (!d_r.isNull())
    {
        d_r->resetLevels(d_r->getCoarsestLevelNumber(), std::min(d_r->getFinestLevelNumber(),d_r->getPatchHierarchy()->getFinestLevelNumber()));
        d_r->freeVectorComponents();
        d_r.setNull();
    }

    // Deallocate operator state.
    d_fac_strategy->deallocateOperatorState();

    // Indicate that the operator is NOT initialized.
    d_is_initialized = false;
    return;
}// deallocateSolverState

/////////////////////////////// PRIVATE //////////////////////////////////////

void
FACPreconditioner::getFromInput(
    tbox::Pointer<tbox::Database> db)
{
    if (db.isNull()) return;

    MGCycleType cycle_type = string_to_enum<MGCycleType>(db->getStringWithDefault("cycle_type", enum_to_string<MGCycleType>(d_cycle_type)));
    setMGCycleType(cycle_type);

    int num_pre_sweeps = db->getIntegerWithDefault("num_pre_sweeps", d_num_pre_sweeps);
    setNumPreSmoothingSweeps(num_pre_sweeps);

    int num_post_sweeps = db->getIntegerWithDefault("num_post_sweeps", d_num_post_sweeps);
    setNumPostSmoothingSweeps(num_post_sweeps);

    bool logging = db->getBoolWithDefault("enable_logging", d_do_log);
    enableLogging(logging);
    return;
}// getFromInput

void
FACPreconditioner::FACVCycleNoPreSmoothing(
    SAMRAIVectorReal<NDIM,double>& u,
    SAMRAIVectorReal<NDIM,double>& f,
    int level_num)
{
    if (level_num == d_coarsest_ln)
    {
        // Solve Au = f on the coarsest level.
        d_fac_strategy->solveCoarsestLevel(u, f, level_num);
    }
    else
    {
        // Restrict the residual to the next coarser level.
        d_fac_strategy->restrictResidual(f, f, level_num-1);

        // Recursively call the FAC algorithm.
        FACVCycleNoPreSmoothing(u, f, level_num-1);

        // Prolong the error from the next coarser level and correct the
        // solution on the current level.
        d_fac_strategy->prolongErrorAndCorrect(u, u, level_num);

        // Smooth error on the current level.
        if (d_num_post_sweeps > 0)
        {
            d_fac_strategy->smoothError(u, f, level_num, d_num_post_sweeps, false, true);
        }
    }
    return;
}// FACVCycleNoPreSmoothing

void
FACPreconditioner::FACVCycle(
    SAMRAIVectorReal<NDIM,double>& u,
    SAMRAIVectorReal<NDIM,double>& f,
    int level_num)
{
    if (level_num == d_coarsest_ln)
    {
        // Solve Au = f on the coarsest level.
        d_fac_strategy->solveCoarsestLevel(u, f, level_num);
        d_recompute_residual = true;
    }
    else
    {
        // Smooth the error on the current level.
        if (d_num_pre_sweeps > 0)
        {
            d_fac_strategy->smoothError(u, f, level_num, d_num_pre_sweeps, true, false);
            d_recompute_residual = true;
        }

        // Compute the composite-grid residual on the current level and the next
        // coarser level, and restrict the residual to the next coarser level.
        if (d_recompute_residual)
        {
            d_fac_strategy->computeResidual(*d_r, u, f, level_num-1, level_num);
            d_fac_strategy->restrictResidual(*d_r, f, level_num-1);
        }
        else
        {
            d_fac_strategy->restrictResidual(f, f, level_num-1);
        }

        // Recursively call the FAC algorithm.
        FACVCycle(u, f, level_num-1);

        // Prolong the error from the next coarser level and correct the
        // solution on level.
        d_fac_strategy->prolongErrorAndCorrect(u, u, level_num);

        // Smooth error on level.
        if (d_num_post_sweeps > 0)
        {
            d_fac_strategy->smoothError(u, f, level_num, d_num_post_sweeps, false, true);
            d_recompute_residual = true;
        }
    }
    return;
}// FACVCycle

void
FACPreconditioner::FACWCycle(
    SAMRAIVectorReal<NDIM,double>& u,
    SAMRAIVectorReal<NDIM,double>& f,
    int level_num)
{
    if (level_num == d_coarsest_ln)
    {
        // Solve Au = f on the coarsest level.
        d_fac_strategy->solveCoarsestLevel(u, f, level_num);
        d_recompute_residual = true;
    }
    else
    {
        // Smooth the error on the current level.
        if (d_num_pre_sweeps > 0)
        {
            d_fac_strategy->smoothError(u, f, level_num, d_num_pre_sweeps, true, false);
            d_recompute_residual = true;
        }

        // Compute the composite-grid residual on the current level and the next
        // coarser level, and restrict the residual to the next coarser level.
        if (d_recompute_residual)
        {
            d_fac_strategy->computeResidual(*d_r, u, f, level_num-1, level_num);
            d_fac_strategy->restrictResidual(*d_r, f, level_num-1);
        }
        else
        {
            d_fac_strategy->restrictResidual(f, f, level_num-1);
        }

        // Recursively call the FAC algorithm.
        FACWCycle(u, f, level_num-1);
        FACWCycle(u, f, level_num-1);

        // Prolong the error from the next coarser level and correct the
        // solution on level.
        d_fac_strategy->prolongErrorAndCorrect(u, u, level_num);

        // Smooth error on level.
        if (d_num_post_sweeps > 0)
        {
            d_fac_strategy->smoothError(u, f, level_num, d_num_post_sweeps, false, true);
            d_recompute_residual = true;
        }
    }
    return;
}// FACWCycle

void
FACPreconditioner::FACFCycle(
    SAMRAIVectorReal<NDIM,double>& u,
    SAMRAIVectorReal<NDIM,double>& f,
    int level_num)
{
    if (level_num == d_coarsest_ln)
    {
        // Solve Au = f on the coarsest level.
        d_fac_strategy->solveCoarsestLevel(u, f, level_num);
        d_recompute_residual = true;
    }
    else
    {
        // Smooth the error on the current level.
        if (d_num_pre_sweeps > 0)
        {
            d_fac_strategy->smoothError(u, f, level_num, d_num_pre_sweeps, true, false);
            d_recompute_residual = true;
        }

        // Compute the composite-grid residual on the current level and the next
        // coarser level, and restrict the residual to the next coarser level.
        if (d_recompute_residual)
        {
            d_fac_strategy->computeResidual(*d_r, u, f, level_num-1, level_num);
            d_fac_strategy->restrictResidual(*d_r, f, level_num-1);
        }
        else
        {
            d_fac_strategy->restrictResidual(f, f, level_num-1);
        }

        // Recursively call the FAC algorithm.
        FACWCycle(u, f, level_num-1);
        FACVCycle(u, f, level_num-1);

        // Prolong the error from the next coarser level and correct the
        // solution on level.
        d_fac_strategy->prolongErrorAndCorrect(u, u, level_num);

        // Smooth error on level.
        if (d_num_post_sweeps > 0)
        {
            d_fac_strategy->smoothError(u, f, level_num, d_num_post_sweeps, false, true);
            d_recompute_residual = true;
        }
    }
    return;
}// FACFCycle

//////////////////////////////////////////////////////////////////////////////

}// namespace IBTK

//////////////////////////////////////////////////////////////////////////////
