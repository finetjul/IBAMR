// Filename: IBEulerianForceFunction.C
// Created on 28 Sep 2004 by Boyce Griffith
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

#include "IBHierarchyIntegrator.h"

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
#include <ibamr/namespaces.h>

// SAMRAI INCLUDES
#include <CellData.h>
#include <PatchCellDataBasicOps.h>
#include <PatchSideDataBasicOps.h>
#include <SideData.h>
#include <tbox/MathUtilities.h>

// C++ STDLIB INCLUDES
#include <limits>

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBAMR
{
/////////////////////////////// STATIC ///////////////////////////////////////

////////////////////////////// PUBLIC ///////////////////////////////////////

IBHierarchyIntegrator::IBEulerianForceFunction::IBEulerianForceFunction(
    const IBHierarchyIntegrator* const ib_solver)
    : CartGridFunction(ib_solver->getName()+"::IBEulerianForceFunction"),
      d_ib_solver(ib_solver)
{
    // intentionally blank
    return;
}// IBEulerianForceFunction

IBHierarchyIntegrator::IBEulerianForceFunction::~IBEulerianForceFunction()
{
    // intentionally blank
    return;
}// ~IBEulerianForceFunction

bool
IBHierarchyIntegrator::IBEulerianForceFunction::isTimeDependent() const
{
    return true;
}// isTimeDependent

void
IBHierarchyIntegrator::IBEulerianForceFunction::setDataOnPatch(
    const int data_idx,
    Pointer<Variable<NDIM> > var,
    Pointer<Patch<NDIM> > patch,
    const double data_time,
    const bool initial_time,
    Pointer<PatchLevel<NDIM> > level)
{
    Pointer<PatchData<NDIM> > f_data = patch->getPatchData(data_idx);
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(!f_data.isNull());
#endif
    Pointer<CellData<NDIM,double> > f_cc_data = f_data;
    Pointer<SideData<NDIM,double> > f_sc_data = f_data;
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(!f_cc_data.isNull() || !f_sc_data.isNull());
#endif
    if (!d_ib_solver->d_body_force_fcn.isNull())
    {
        d_ib_solver->d_body_force_fcn->setDataOnPatch(data_idx, var, patch, data_time, initial_time, level);
    }
    else
    {
        if (!f_cc_data.isNull()) f_cc_data->fillAll(0.0);
        if (!f_sc_data.isNull()) f_sc_data->fillAll(0.0);
    }

    if (initial_time) return;

    Pointer<PatchData<NDIM> > f_ib_data = patch->getPatchData(d_ib_solver->d_f_idx);
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(!f_ib_data.isNull());
#endif
    Pointer<CellData<NDIM,double> > f_ib_cc_data = f_ib_data;
    Pointer<SideData<NDIM,double> > f_ib_sc_data = f_ib_data;
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(!f_ib_cc_data.isNull() || !f_ib_sc_data.isNull());
    TBOX_ASSERT((!f_ib_cc_data.isNull() && !f_cc_data.isNull()) || (!f_ib_sc_data.isNull() && !f_sc_data.isNull()));
#endif
    if (!f_cc_data.isNull())
    {
        PatchCellDataBasicOps<NDIM,double> patch_ops;
        patch_ops.add(f_cc_data, f_cc_data, f_ib_cc_data, patch->getBox());
    }
    if (!f_sc_data.isNull())
    {
        PatchSideDataBasicOps<NDIM,double> patch_ops;
        patch_ops.add(f_sc_data, f_sc_data, f_ib_sc_data, patch->getBox());
    }
    return;
}// setDataOnPatch

/////////////////////////////// PROTECTED ////////////////////////////////////

/////////////////////////////// PRIVATE //////////////////////////////////////

/////////////////////////////// NAMESPACE ////////////////////////////////////

} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////
