#ifndef included_IBTargetPointForceGen
#define included_IBTargetPointForceGen

// Filename: IBTargetPointForceGen.h
// Last modified: <30.Dec.2009 19:51:03 griffith@boyce-griffiths-mac-pro.local>
// Created on 21 Mar 2007 by Boyce Griffith (griffith@box221.cims.nyu.edu)

/////////////////////////////// INCLUDES /////////////////////////////////////

// IBAMR INCLUDES
#include <ibamr/IBLagrangianForceStrategy.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBAMR
{

/*!
 * \brief Class IBTargetPointForceGen is a concrete IBLagrangianForceStrategy
 * that computes the penalty forces generated by a collection of fixed target
 * points (i.e., forces that approximately impose Dirichlet boundary conditions
 * at the nodes of the Lagrangian mesh).
 *
 * \note Class IBTargetPointForceGen DOES NOT correct for periodic displacements
 * of IB points; however, when used in conjuction with class IBStandardForceGen,
 * periodic displacements are handled correctly.
 */
class IBTargetPointForceGen
    : public IBLagrangianForceStrategy
{
public:
    /*!
     * \brief Default constructor.
     */
    IBTargetPointForceGen(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> input_db=NULL);

    /*!
     * \brief Virtual destructor.
     */
    virtual
    ~IBTargetPointForceGen();

    /*!
     * \brief Compute the penalty forces determined with the present
     * configuration of the Lagrangian mesh.
     *
     * \note Nodal forces computed by this method are \em added to the force
     * vector.
     */
    virtual void
    computeLagrangianForce(
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> F_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> X_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> U_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the non-zero structure of the force Jacobian matrix.
     *
     * \note Elements indices must be global PETSc indices.
     */
    virtual void
    computeLagrangianForceJacobianNonzeroStructure(
        std::vector<int>& d_nnz,
        std::vector<int>& o_nnz,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the Jacobian of the force with respect to the present
     * structure configuration.
     *
     * \note The elements of the Jacobian should be "accumulated" in the
     * provided matrix J.
     */
    virtual void
    computeLagrangianForceJacobian(
        Mat& J_mat,
        MatAssemblyType assembly_type,
        const double X_coef,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> X_data,
        const double U_coef,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> U_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

    /*!
     * \brief Compute the potential energy with respect to the present structure
     * configuration and velocity.
     *
     * \note This method is not actually implemented --- it just prints a
     * warning message and returns 0.0.
     */
    virtual double
    computeLagrangianEnergy(
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> X_data,
        SAMRAI::tbox::Pointer<IBTK::LNodeLevelData> U_data,
        const SAMRAI::tbox::Pointer<SAMRAI::hier::PatchHierarchy<NDIM> > hierarchy,
        const int level_number,
        const double data_time,
        IBTK::LDataManager* const lag_manager);

private:
    /*!
     * \brief Copy constructor.
     *
     * \note This constructor is not implemented and should not be used.
     *
     * \param from The value to copy to this object.
     */
    IBTargetPointForceGen(
        const IBTargetPointForceGen& from);

    /*!
     * \brief Assignment operator.
     *
     * \note This operator is not implemented and should not be used.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    IBTargetPointForceGen&
    operator=(
        const IBTargetPointForceGen& that);

    /*!
     * \brief Read input values, indicated above, from given database.
     *
     * The database pointer may be null.
     */
    void
    getFromInput(
        SAMRAI::tbox::Pointer<SAMRAI::tbox::Database> db);
};
}// namespace IBAMR

/////////////////////////////// INLINE ///////////////////////////////////////

//#include <ibamr/IBTargetPointForceGen.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_IBTargetPointForceGen
