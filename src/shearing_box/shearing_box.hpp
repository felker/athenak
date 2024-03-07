#ifndef SHEARING_BOX_SHEARING_BOX_HPP_
#define SHEARING_BOX_SHEARING_BOX_HPP_
//========================================================================================
// AthenaK astrophysical fluid dynamics code
// Copyright(C) 2020 James M. Stone <jmstone@ias.edu> and the Athena code team
// Licensed under the 3-clause BSD License (the "LICENSE")
//========================================================================================
//! \file shearing_box.hpp
//  \brief definitions for ShearingBox class

#include "athena.hpp"
#include "parameter_input.hpp"
#include "tasklist/task_list.hpp"
#include "bvals/bvals.hpp"

// forward declarations


//----------------------------------------------------------------------------------------
//! \struct ShearingBoxTaskIDs
//  \brief container to hold TaskIDs of all shearing_box tasks

struct ShearingBoxTaskIDs {
  TaskID irecv;
  TaskID copyu;
  TaskID flux;
  TaskID sendf;
  TaskID recvf;
  TaskID csend;
  TaskID crecv;
};

//----------------------------------------------------------------------------------------
//! \struct ShearingBoxBuffer
//! \brief container for storage for boundary buffers with the shearing box
//! Basically a much simplified version of the BoundaryBuffer struct.

struct ShearingBoxBuffer {
  // 5D Views that store buffer data on device
  DvceArray5D<Real> vars, flds;
#if MPI_PARALLEL_ENABLED
  // vectors of length (number of MBs) to hold MPI requests
  // Using STL vector causes problems with some GPU compilers, so just use plain C array
  MPI_Request *vars_req, *flds_req;
#endif
};

//----------------------------------------------------------------------------------------
//! \class ShearingBox

class ShearingBox {
 public:
  ShearingBox(MeshBlockPack *ppack, ParameterInput *pin, int nvar);
  ~ShearingBox();

  // data
  Real qshear, omega0;      // shearing box parameters
  int maxjshift;            // maximum integer shift of any cell in orbital advection
  // ***following is not yet implemented***
  bool shearing_box_r_phi;  // indicates calculation in 2D (r-\phi) plane

  // container to hold names of TaskIDs
  ShearingBoxTaskIDs id;

  // data buffers for orbital advection. Only two x2-faces communicate
  ShearingBoxBuffer sendbuf_orb[2], recvbuf_orb[2];

#if MPI_PARALLEL_ENABLED
  // unique MPI communicators for orbital advection and shearing box
  MPI_Comm comm_orb;
#endif

  // functions to add source terms
  void SrcTerms(DvceArray5D<Real> &u0, const DvceArray5D<Real> &w0, const Real bdt);
  void SrcTerms(DvceArray5D<Real> &u0, const DvceArray5D<Real> &w0,
                const DvceArray5D<Real> &bcc0, const Real bdt);
  void EFieldSrcTerms(const DvceFaceFld4D<Real> &b0, DvceEdgeFld4D<Real> &efld);

  // functions to communicate CC data with orbital advection
  TaskStatus PackAndSendCC_Orb(DvceArray5D<Real> &a);
  TaskStatus RecvAndUnpackCC_Orb(DvceArray5D<Real> &a, ReconstructionMethod rcon);
  // functions to communicate FC data with orbital advection
  TaskStatus PackAndSendFC_Orb(DvceFaceFld4D<Real> &b);
  TaskStatus RecvAndUnpackFC_Orb(DvceFaceFld4D<Real> &b0, ReconstructionMethod rcon);

 private:
  MeshBlockPack *pmy_pack;
};
#endif // SHEARING_BOX_SHEARING_BOX_HPP_
