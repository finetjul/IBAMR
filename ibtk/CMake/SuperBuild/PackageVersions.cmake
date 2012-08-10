set(IBTK_DEPENDENCY_URL "/home/rortiz/Downloads" CACHE PATH "Path for external packages." )

set(LAPACK_MAJOR 3)
set(LAPACK_MINOR 4)
set(LAPACK_PATCH 0)
set(LAPACK_VERSION ${LAPACK_MAJOR}.${LAPACK_MINOR}.${LAPACK_PATCH})
set(LAPACK_URL http://www.netlib.org/lapack)
set(LAPACK_GZ lapack-${LAPACK_VERSION}.tgz)
set(LAPACK_MD5 02d5706ec03ba885fc246e5fa10d8c70)

set(OPENMPI_MAJOR 1)
set(OPENMPI_MINOR 6)
set(OPENMPI_PATCH )
set(OPENMPI_VERSION ${OPENMPI_MAJOR}.${OPENMPI_MINOR})
set(OPENMPI_URL http://www.open-mpi.org/software/ompi/v${OPENMPI_MAJOR}.${OPENMPI_MINOR}/downloads)
set(OPENMPI_GZ openmpi-${OPENMPI_VERSION}.tar.gz)
set(OPENMPI_MD5 3ed0c892a0c921270cb9c7af2fdfd2d2)

set(SILO_MAJOR 4)
set(SILO_MINOR 8)
set(SILO_PATCH )
set(SILO_VERSION ${SILO_MAJOR}.${SILO_MINOR}.${SILO_PATCH})
set(SILO_URL ${IBTK_DEPENDENCY_URL})
set(SILO_GZ silo-${SILO_MAJOR}.${SILO_MINOR}-bsd.tar.gz)
set(SILO_MD5 03e27c977f34dc6e9a5f3864153c24fe)

set(QD_MAJOR 2)
set(QD_MINOR 3)
set(QD_PATCH 13)
set(QD_VERSION ${QD_MAJOR}.${QD_MINOR}.${QD_PATCH})
set(QD_URL http://crd.lbl.gov/~dhbailey/mpdist)
set(QD_GZ qd-${QD_VERSION}.tar.gz)
set(QD_MD5 1c901295624d91df0114614f2ccf914f)

set(MUPARSER_MAJOR 2)
set(MUPARSER_MINOR 2)
set(MUPARSER_PATCH 2)
set(MUPARSER_VERSION ${MUPARSER_MAJOR}_${MUPARSER_MINOR}_${MUPARSER_PATCH})
set(MUPARSER_URL ${IBTK_DEPENDENCY_URL})
set(MUPARSER_GZ muparser_v${MUPARSER_VERSION}.zip)
set(MUPARSER_MD5 6d77b5cb8096fe2c50afe36ad41bc14a)

set(HDF5_MAJOR 1)
set(HDF5_MINOR 8)
set(HDF5_PATCH 9)
set(HDF5_VERSION ${HDF5_MAJOR}.${HDF5_MINOR}.${HDF5_PATCH})
set(HDF5_URL http://www.hdfgroup.org/ftp/HDF5/current/src)
set(HDF5_GZ hdf5-${HDF5_VERSION}.tar.gz)
set(HDF5_MD5 d1266bb7416ef089400a15cc7c963218)

set(LIBMESH_MAJOR 0)
set(LIBMESH_MINOR 7)
set(LIBMESH_PATCH 3.1)
set(LIBMESH_VERSION ${LIBMESH_MAJOR}.${LIBMESH_MINOR}.${LIBMESH_PATCH})
set(LIBMESH_URL ${IBTK_DEPENDENCY_URL})
set(LIBMESH_GZ libmesh-${LIBMESH_VERSION}.tar.gz)
set(LIBMESH_MD5 bb5bf351c39deeac00692f2a9ebb5590)

set(HYPRE_MAJOR 2)
set(HYPRE_MINOR 8)
set(HYPRE_PATCH 0)
set(HYPRE_VERSION ${HYPRE_MAJOR}.${HYPRE_MINOR}.${HYPRE_PATCH}b)
set(HYPRE_URL ${IBTK_DEPENDENCY_URL})
set(HYPRE_GZ hypre-${HYPRE_VERSION}.tar.gz)
set(HYPRE_MD5 6b4db576c68d2072e48efbc00ea58489)

set(BLITZ_MAJOR 0)
set(BLITZ_MINOR 9)
set(BLITZ_VERSION ${BLITZ_MAJOR}.${BLITZ_MINOR})
set(BLITZ_URL ${IBTK_DEPENDENCY_URL})
set(BLITZ_GZ blitz-${BLITZ_VERSION}.tar.gz)
set(BLITZ_MD5 031df2816c73e2d3bd6d667bbac19eca)

set(SAMRAI_MAJOR 2)
set(SAMRAI_MINOR 4)
set(SAMRAI_PATCH 4)
set(SAMRAI_VERSION ${SAMRAI_MAJOR}.${SAMRAI_MINOR}.${SAMRAI_PATCH})
set(SAMRAI_URL ${IBTK_DEPENDENCY_URL})
set(SAMRAI_GZ SAMRAI-v${SAMRAI_VERSION}.tar.gz)
set(SAMRAI_MD5 04fb048ed0efe7c531ac10c81cc5f6ac)

set(PETSC_MAJOR 3)
set(PETSC_MINOR 3)
set(PETSC_PATCH 2)
set(PETSC_VERSION ${PETSC_MAJOR}.${PETSC_MINOR}-p${PETSC_PATCH})
set(PETSC_URL http://ftp.mcs.anl.gov/pub/petsc/release-snapshots)
set(PETSC_GZ petsc-${PETSC_VERSION}.tar.gz)
set(PETSC_MD5 303e2a95ba0b8103ff4b1c8cd6a96db9)
