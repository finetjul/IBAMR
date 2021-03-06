// Filename: IBSpringForceSpec.I
// Created on 11 Jun 2007 by Boyce Griffith
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

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBTK INCLUDES
#include <ibtk/StreamableManager.h>

// SAMRAI INCLUDES
#include <tbox/PIO.h>
#include <tbox/Utilities.h>

/////////////////////////////// NAMESPACE ////////////////////////////////////

namespace IBAMR
{
/////////////////////////////// STATIC ///////////////////////////////////////

inline bool
IBSpringForceSpec::getIsRegisteredWithStreamableManager()
{
    return (STREAMABLE_CLASS_ID != IBTK::StreamableManager::getUnregisteredID());
}// getIsRegisteredWithStreamableManager

/////////////////////////////// PUBLIC ///////////////////////////////////////

inline
IBSpringForceSpec::IBSpringForceSpec(
    const unsigned int num_springs)
    : d_master_idx(-1),
      d_slave_idxs(num_springs),
      d_force_fcn_idxs(num_springs),
      d_stiffnesses(num_springs),
      d_rest_lengths(num_springs)
#if ENABLE_SUBDOMAIN_INDICES
    , d_subdomain_idxs(num_springs)
#endif
{
#ifdef DEBUG_CHECK_ASSERTIONS
    if (!getIsRegisteredWithStreamableManager())
    {
        TBOX_ERROR("IBSpringForceSpec::IBSpringForceSpec():\n"
                   << "  must call IBSpringForceSpec::registerWithStreamableManager() before\n"
                   << "  creating any IBSpringForceSpec objects.\n");
    }
#endif
    return;
}// IBSpringForceSpec

inline
IBSpringForceSpec::IBSpringForceSpec(
    const int master_idx,
    const std::vector<int>& slave_idxs,
    const std::vector<int>& force_fcn_idxs,
    const std::vector<double>& stiffnesses,
    const std::vector<double>& rest_lengths
#if ENABLE_SUBDOMAIN_INDICES
    ,const std::vector<int>& subdomain_idxs
#endif
                                     )
    : d_master_idx(master_idx),
      d_slave_idxs(slave_idxs),
      d_force_fcn_idxs(force_fcn_idxs),
      d_stiffnesses(stiffnesses),
      d_rest_lengths(rest_lengths)
#if ENABLE_SUBDOMAIN_INDICES
    , d_subdomain_idxs(subdomain_idxs)
#endif
{
#ifdef DEBUG_CHECK_ASSERTIONS
    const unsigned int num_springs = d_slave_idxs.size();
    TBOX_ASSERT(num_springs == d_force_fcn_idxs.size());
    TBOX_ASSERT(num_springs == d_stiffnesses.size());
    TBOX_ASSERT(num_springs == d_rest_lengths.size());
#if ENABLE_SUBDOMAIN_INDICES
    TBOX_ASSERT(num_springs == d_subdomain_idxs.size());
#endif
    if (!getIsRegisteredWithStreamableManager())
    {
        TBOX_ERROR("IBSpringForceSpec::IBSpringForceSpec():\n"
                   << "  must call IBSpringForceSpec::registerWithStreamableManager() before\n"
                   << "  creating any IBSpringForceSpec objects.\n");
    }
#endif
    return;
}// IBSpringForceSpec

inline
IBSpringForceSpec::~IBSpringForceSpec()
{
    // intentionally blank
    return;
}// ~IBSpringForceSpec

inline unsigned int
IBSpringForceSpec::getNumberOfSprings() const
{
    const unsigned int num_springs = d_slave_idxs.size();
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(num_springs == d_force_fcn_idxs.size());
    TBOX_ASSERT(num_springs == d_stiffnesses.size());
    TBOX_ASSERT(num_springs == d_rest_lengths.size());
#endif
    return num_springs;
}// getNumberOfSprings

inline const int&
IBSpringForceSpec::getMasterNodeIndex() const
{
    return d_master_idx;
}// getMasterNodeIndex

inline int&
IBSpringForceSpec::getMasterNodeIndex()
{
    return d_master_idx;
}// getMasterNodeIndex

inline const std::vector<int>&
IBSpringForceSpec::getSlaveNodeIndices() const
{
    return d_slave_idxs;
}// getSlaveNodeIndices

inline std::vector<int>&
IBSpringForceSpec::getSlaveNodeIndices()
{
    return d_slave_idxs;
}// getSlaveNodeIndices

inline const std::vector<int>&
IBSpringForceSpec::getForceFunctionIndices() const
{
    return d_force_fcn_idxs;
}// getForceFunctionIndices

inline std::vector<int>&
IBSpringForceSpec::getForceFunctionIndices()
{
    return d_force_fcn_idxs;
}// getForceFunctionIndices

inline const std::vector<double>&
IBSpringForceSpec::getStiffnesses() const
{
    return d_stiffnesses;
}// getStiffnesses

inline std::vector<double>&
IBSpringForceSpec::getStiffnesses()
{
    return d_stiffnesses;
}// getStiffnesses

inline const std::vector<double>&
IBSpringForceSpec::getRestingLengths() const
{
    return d_rest_lengths;
}// getRestingLengths

inline std::vector<double>&
IBSpringForceSpec::getRestingLengths()
{
    return d_rest_lengths;
}// getRestingLengths

#if ENABLE_SUBDOMAIN_INDICES
inline const std::vector<int>&
IBSpringForceSpec::getSubdomainIndices() const
{
    return d_subdomain_idxs;
}// getSubdomainIndices

inline std::vector<int>&
IBSpringForceSpec::getSubdomainIndices()
{
    return d_subdomain_idxs;
}// getSubdomainIndices
#endif

inline int
IBSpringForceSpec::getStreamableClassID() const
{
    return STREAMABLE_CLASS_ID;
}// getStreamableClassID

inline size_t
IBSpringForceSpec::getDataStreamSize() const
{
    const unsigned int num_springs = d_slave_idxs.size();
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(num_springs == d_force_fcn_idxs.size());
    TBOX_ASSERT(num_springs == d_stiffnesses.size());
    TBOX_ASSERT(num_springs == d_rest_lengths.size());
#endif
    return ((2+2*num_springs)*SAMRAI::tbox::AbstractStream::sizeofInt() +
            (  2*num_springs)*SAMRAI::tbox::AbstractStream::sizeofDouble())
#if ENABLE_SUBDOMAIN_INDICES
        + num_springs*SAMRAI::tbox::AbstractStream::sizeofInt()
#endif
        ;
}// getDataStreamSize

inline void
IBSpringForceSpec::packStream(
    SAMRAI::tbox::AbstractStream& stream)
{
    const unsigned int num_springs = d_slave_idxs.size();
#ifdef DEBUG_CHECK_ASSERTIONS
    TBOX_ASSERT(num_springs == d_force_fcn_idxs.size());
    TBOX_ASSERT(num_springs == d_stiffnesses.size());
    TBOX_ASSERT(num_springs == d_rest_lengths.size());
#endif
    stream << static_cast<int>(num_springs);
    stream.pack(&d_master_idx,1);
    stream.pack(&d_slave_idxs[0],num_springs);
    stream.pack(&d_force_fcn_idxs[0],num_springs);
    stream.pack(&d_stiffnesses[0],num_springs);
    stream.pack(&d_rest_lengths[0],num_springs);
#if ENABLE_SUBDOMAIN_INDICES
    stream.pack(&d_subdomain_idxs[0],num_springs);
#endif
    return;
}// packStream

/////////////////////////////// PROTECTED ////////////////////////////////////

/////////////////////////////// PRIVATE //////////////////////////////////////

/////////////////////////////// NAMESPACE ////////////////////////////////////

} // namespace IBAMR

//////////////////////////////////////////////////////////////////////////////
