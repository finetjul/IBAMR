c
c     Routines to coarsen values via high-order interpolation.
c
c     Created on 02 May 2008 by Boyce Griffith
c
c     Copyright (c) 2002-2010, Boyce Griffith
c     All rights reserved.
c
c     Redistribution and use in source and binary forms, with or without
c     modification, are permitted provided that the following conditions
c     are met:
c
c        * Redistributions of source code must retain the above
c          copyright notice, this list of conditions and the following
c          disclaimer.
c
c        * Redistributions in binary form must reproduce the above
c          copyright notice, this list of conditions and the following
c          disclaimer in the documentation and/or other materials
c          provided with the distribution.
c
c        * Neither the name of New York University nor the names of its
c          contributors may be used to endorse or promote products
c          derived from this software without specific prior written
c          permission.
c
c     THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
c     CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
c     INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
c     MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
c     DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
c     BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
c     EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
c     TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
c     DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
c     ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
c     TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
c     THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
c     SUCH DAMAGE.
c
define(NDIM,3)dnl
define(REAL,`double precision')dnl
define(INTEGER,`integer')dnl
include(SAMRAI_FORTDIR/pdat_m4arrdim3d.i)dnl
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Coarsen cell-centered data via cubic interpolation.
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      subroutine cccubiccoarsen3d(
     &     U_crse,U_crse_gcw,
     &     U_fine,U_fine_gcw,
     &     ilowerc0,iupperc0,
     &     ilowerc1,iupperc1,
     &     ilowerc2,iupperc2,
     &     ilowerf0,iupperf0,
     &     ilowerf1,iupperf1,
     &     ilowerf2,iupperf2,
     &     ratio_to_coarser,
     &     coarse_box_lower,coarse_box_upper)
c
      implicit none
c
c     Constants.
c
      REAL wgts(-2:1)
      DATA wgts/-6.25d-2,5.625d-1,5.625d-1,-6.25d-2/
c
c     Input.
c
      INTEGER ilowerc0,iupperc0
      INTEGER ilowerc1,iupperc1
      INTEGER ilowerc2,iupperc2
      INTEGER ilowerf0,iupperf0
      INTEGER ilowerf1,iupperf1
      INTEGER ilowerf2,iupperf2
      INTEGER U_crse_gcw,U_fine_gcw

      INTEGER ratio_to_coarser(0:NDIM-1)

      INTEGER coarse_box_lower(0:NDIM-1), coarse_box_upper(0:NDIM-1)

      REAL U_fine(CELL3d(ilowerf,iupperf,U_fine_gcw))
c
c     Input/Output.
c
      REAL U_crse(CELL3d(ilowerc,iupperc,U_crse_gcw))
c
c     Local variables.
c
      INTEGER i,i_f,i_c
      INTEGER j,j_f,j_c
      INTEGER k,k_f,k_c
c
c     Coarsen the fine data via cubic interpolation.
c
      do k_c = coarse_box_lower(2),coarse_box_upper(2)
         do j_c = coarse_box_lower(1),coarse_box_upper(1)
            do i_c = coarse_box_lower(0),coarse_box_upper(0)
               U_crse(i_c,j_c,k_c) = 0.d0
            enddo
         enddo
      enddo
      do k_c = coarse_box_lower(2),coarse_box_upper(2)
         k_f = k_c*ratio_to_coarser(2)
         do k = -2,1
            do j_c = coarse_box_lower(1),coarse_box_upper(1)
               j_f = j_c*ratio_to_coarser(1)
               do j = -2,1
                  do i_c = coarse_box_lower(0),coarse_box_upper(0)
                     i_f = i_c*ratio_to_coarser(0)
                     do i = -2,1
                        U_crse(i_c,j_c,k_c) = U_crse(i_c,j_c,k_c) +
     &                       wgts(i)*wgts(j)*wgts(k)*U_fine(
     &                       i_f+ratio_to_coarser(0)/2+i,
     &                       j_f+ratio_to_coarser(1)/2+j,
     &                       k_f+ratio_to_coarser(2)/2+k)
                     enddo
                  enddo
               enddo
            enddo
         enddo
      enddo
c
      return
      end
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
c     Coarsen side-centered data via cubic interpolation.
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
c
      subroutine sccubiccoarsen3d(
     &     U_crse0,U_crse1,U_crse2,U_crse_gcw,
     &     U_fine0,U_fine1,U_fine2,U_fine_gcw,
     &     ilowerc0,iupperc0,
     &     ilowerc1,iupperc1,
     &     ilowerc2,iupperc2,
     &     ilowerf0,iupperf0,
     &     ilowerf1,iupperf1,
     &     ilowerf2,iupperf2,
     &     ratio_to_coarser,
     &     coarse_box_lower,coarse_box_upper)
c
      implicit none
c
c     Constants.
c
      REAL wgts(-2:1)
      DATA wgts/-6.25d-2,5.625d-1,5.625d-1,-6.25d-2/
c
c     Input.
c
      INTEGER ilowerc0,iupperc0
      INTEGER ilowerc1,iupperc1
      INTEGER ilowerc2,iupperc2
      INTEGER ilowerf0,iupperf0
      INTEGER ilowerf1,iupperf1
      INTEGER ilowerf2,iupperf2
      INTEGER U_crse_gcw,U_fine_gcw

      INTEGER ratio_to_coarser(0:NDIM-1)

      INTEGER coarse_box_lower(0:NDIM-1), coarse_box_upper(0:NDIM-1)

      REAL U_fine0(
     &     SIDE3d0(ilowerf,iupperf,U_fine_gcw)
     &     )
      REAL U_fine1(
     &     SIDE3d1(ilowerf,iupperf,U_fine_gcw)
     &     )
      REAL U_fine2(
     &     SIDE3d2(ilowerf,iupperf,U_fine_gcw)
     &     )
c
c     Input/Output.
c
      REAL U_crse0(
     &     SIDE3d0(ilowerc,iupperc,U_crse_gcw)
     &     )
      REAL U_crse1(
     &     SIDE3d1(ilowerc,iupperc,U_crse_gcw)
     &     )
      REAL U_crse2(
     &     SIDE3d2(ilowerc,iupperc,U_crse_gcw)
     &     )
c
c     Local variables.
c
      INTEGER i,j,k,i_f,i_c,j_f,j_c,k_f,k_c
c
c     Coarsen the fine data via cubic interpolation.
c
      do k_c = coarse_box_lower(2),coarse_box_upper(2)
         do j_c = coarse_box_lower(1),coarse_box_upper(1)
            do i_c = coarse_box_lower(0),coarse_box_upper(0)+1
               U_crse0(i_c,j_c,k_c) = 0.d0
            enddo
         enddo
      enddo
      do k_c = coarse_box_lower(2),coarse_box_upper(2)
         k_f = k_c*ratio_to_coarser(2)
         do k = -2,1
            do j_c = coarse_box_lower(1),coarse_box_upper(1)
               j_f = j_c*ratio_to_coarser(1)
               do j = -2,1
                  do i_c = coarse_box_lower(0),coarse_box_upper(0)+1
                     i_f = i_c*ratio_to_coarser(0)
                     U_crse0(i_c,j_c,k_c) = U_crse0(i_c,j_c,k_c) +
     &                    wgts(j)*wgts(k)*U_fine0(
     &                    i_f,
     &                    j+j_f+ratio_to_coarser(1)/2,
     &                    k+k_f+ratio_to_coarser(2)/2)
                  enddo
               enddo
            enddo
         enddo
      enddo
      do k_c = coarse_box_lower(2),coarse_box_upper(2)
         do j_c = coarse_box_lower(1),coarse_box_upper(1)+1
            do i_c = coarse_box_lower(0),coarse_box_upper(0)
               U_crse1(i_c,j_c,k_c) = 0.d0
            enddo
         enddo
      enddo
      do k_c = coarse_box_lower(2),coarse_box_upper(2)
         k_f = k_c*ratio_to_coarser(2)
         do k = -2,1
            do j_c = coarse_box_lower(1),coarse_box_upper(1)+1
               j_f = j_c*ratio_to_coarser(1)
               do i_c = coarse_box_lower(0),coarse_box_upper(0)
                  i_f = i_c*ratio_to_coarser(0)
                  do i = -2,1
                     U_crse1(i_c,j_c,k_c) = U_crse1(i_c,j_c,k_c) +
     &                    wgts(i)*wgts(k)*U_fine1(
     &                    i+i_f+ratio_to_coarser(0)/2,
     &                    j_f,
     &                    k+k_f+ratio_to_coarser(2)/2)
                  enddo
               enddo
            enddo
         enddo
      enddo
      do k_c = coarse_box_lower(2),coarse_box_upper(2)+1
         do j_c = coarse_box_lower(1),coarse_box_upper(1)
            do i_c = coarse_box_lower(0),coarse_box_upper(0)
               U_crse2(i_c,j_c,k_c) = 0.d0
               enddo
            enddo
         enddo
      do k_c = coarse_box_lower(2),coarse_box_upper(2)+1
         k_f = k_c*ratio_to_coarser(2)
         do j_c = coarse_box_lower(1),coarse_box_upper(1)
            j_f = j_c*ratio_to_coarser(1)
            do j = -2,1
               do i_c = coarse_box_lower(0),coarse_box_upper(0)
                  i_f = i_c*ratio_to_coarser(0)
                  do i = -2,1
                     U_crse2(i_c,j_c,k_c) = U_crse2(i_c,j_c,k_c) +
     &                    wgts(i)*wgts(j)*U_fine2(
     &                    i+i_f+ratio_to_coarser(0)/2,
     &                    j+j_f+ratio_to_coarser(1)/2,
     &                    k_f)
                  enddo
               enddo
            enddo
         enddo
      enddo
c
      return
      end
c
ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc
