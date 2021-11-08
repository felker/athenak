//========================================================================================
// Athena++ astrophysical MHD code
// Copyright(C) 2014 James M. Stone <jmstone@princeton.edu> and other code contributors
// Licensed under the 3-clause BSD License, see LICENSE file for details
//========================================================================================
//! \file meshblock.cpp
//  \brief implementation of constructor and functions in MeshBlock class

#include <cstdlib>
#include <iostream>

#include "athena.hpp"
#include "parameter_input.hpp"
#include "mesh.hpp"

//----------------------------------------------------------------------------------------
// MeshBlock constructor:
//

MeshBlock::MeshBlock(MeshBlockPack* ppack, int igids, int nmb) : 
  pmy_pack(ppack), nmb(nmb),
  mbgid("mbgid",nmb),
  mblev("mblev",nmb),
  mbbcs("mbbcs",nmb,6),
  mbcost("lbcost",nmb)
{
  Mesh* pm = pmy_pack->pmesh;
  // initialize host arrays of gids, sizes, bcs over all MeshBlocks
  for (int m=0; m<nmb; ++m) {
    mbgid.h_view(m) = igids + m;
    mblev.h_view(m) = pm->lloclist[igids+m].level;

    // calculate physical size and set BCs of MeshBlock in x1, depending on whether there
    // are one or more MeshBlocks in this direction.
    std::int32_t &lx1 = pm->lloclist[igids+m].lx1;
    std::int32_t &lev = pm->lloclist[igids+m].level;
    std::int32_t nmbx1 = pm->nmb_rootx1 << (lev - pm->root_level);
    if (lx1 == 0) {
      mbbcs(m,0) = pm->mesh_bcs[BoundaryFace::inner_x1];
    } else {
      mbbcs(m,0) = BoundaryFlag::block;
    }

    if (lx1 == nmbx1 - 1) {
      mbbcs(m,1) = pm->mesh_bcs[BoundaryFace::outer_x1];
    } else {
      mbbcs(m,1) = BoundaryFlag::block;
    }

    // calculate physical size and set BCs of MeshBlock in x2, dependng on whether there
    // are none (1D), one, or more MeshBlocks in this direction
    if (pm->mesh_indcs.nx2 == 1) {
      mbbcs(m,2) = pm->mesh_bcs[BoundaryFace::inner_x2];
      mbbcs(m,3) = pm->mesh_bcs[BoundaryFace::outer_x2];
    } else {

      std::int32_t &lx2 = pm->lloclist[igids+m].lx2;
      std::int32_t nmbx2 = pm->nmb_rootx2 << (lev - pm->root_level);
      if (lx2 == 0) {
        mbbcs(m,2) = pm->mesh_bcs[BoundaryFace::inner_x2];
      } else {
        mbbcs(m,2) = BoundaryFlag::block;
      }

      if (lx2 == (nmbx2) - 1) {
        mbbcs(m,3) = pm->mesh_bcs[BoundaryFace::outer_x2];
      } else {
        mbbcs(m,3) = BoundaryFlag::block;
      }

    }

    // calculate physical size and set BCs of MeshBlock in x3, dependng on whether there
    // are none (1D/2D), one, or more MeshBlocks in this direction
    if (pm->mesh_indcs.nx3 == 1) {
      mbbcs(m,4) = pm->mesh_bcs[BoundaryFace::inner_x3];
      mbbcs(m,5) = pm->mesh_bcs[BoundaryFace::outer_x3];
    } else {
      std::int32_t &lx3 = pm->lloclist[igids+m].lx3;
      std::int32_t nmbx3 = pm->nmb_rootx3 << (lev - pm->root_level);
      if (lx3 == 0) {
        mbbcs(m,4) = pm->mesh_bcs[BoundaryFace::inner_x3];
      } else {
        mbbcs(m,4) = BoundaryFlag::block;
      }
      if (lx3 == (nmbx3) - 1) {
        mbbcs(m,5) = pm->mesh_bcs[BoundaryFace::outer_x3];
      } else {
        mbbcs(m,5) = BoundaryFlag::block;
      }
    }

  }

  // For each DualArray: mark host views as modified, and then sync to device array
  mbgid.template modify<HostMemSpace>();
  mblev.template modify<HostMemSpace>();

  mbgid.template sync<DevExeSpace>();
  mblev.template sync<DevExeSpace>();
}

//----------------------------------------------------------------------------------------
// MeshBlock constructor for restarts

//----------------------------------------------------------------------------------------
// MeshBlock destructor

MeshBlock::~MeshBlock()
{
}

//----------------------------------------------------------------------------------------
// \!fn void MeshBlock::FindAndSetNeighbors()
// \brief Search and set all the neighbor blocks
// Information about Neighbors are stored in a 3D array, with the last index storing:
//   mbnghbr(m,n,0) = gid     global ID of neighbor
//   mbnghbr(m,n,1) = level   logical level " "
//   mbnghbr(m,n,2) = rank    MPI rank " "
//   mbnghbr(m,n,3) = destn   index of target recv buffer for comms

void MeshBlock::SetNeighbors(std::unique_ptr<MeshBlockTree> &ptree, int *ranklist)
{
  if (pmy_pack->pmesh->one_d) {nnghbr = 2;}
  if (pmy_pack->pmesh->two_d) {nnghbr = 8;}
  if (pmy_pack->pmesh->three_d) {nnghbr = 26;}

  // allocate size of DualArrays
  Kokkos::realloc(nghbr, nmb, nnghbr);

  // Initialize host view elements of DualViews
  for (int n=0; n<nnghbr; ++n) {
    for (int m=0; m<nmb; ++m) {
      nghbr.h_view(m,n).gid = -1;
      nghbr.h_view(m,n).lev = -1;
      nghbr.h_view(m,n).rank = -1;
      nghbr.h_view(m,n).destn = -1;
    }
  }

  // Search MeshBlock tree and find neighbors
  for (int b=0; b<nmb; ++b) {
    LogicalLocation loc = pmy_pack->pmesh->lloclist[mbgid.h_view(b)];

    // neighbors on x1face
    for (int n=-1; n<=1; n+=2) {
      MeshBlockTree* nt = ptree->FindNeighbor(loc, n, 0, 0);
      if (nt != nullptr) {
        if (nt->pleaf_ != nullptr) {  // neighbor at finer level
          int fface = 1 - (n + 1)/2; // 0 for BoundaryFace::outer_x1, 1 for inner_x1
          MeshBlockTree* nf = nt->GetLeaf(fface, 0, 0);
          nghbr.h_view(b,(1+n)/2).gid = nf->gid_;
          nghbr.h_view(b,(1+n)/2).lev = nf->loc_.level;
          nghbr.h_view(b,(1+n)/2).rank = ranklist[nf->gid_];
          nghbr.h_view(b,(1+n)/2).destn = (1-n)/2;
        } else {
          nghbr.h_view(b,(1+n)/2).gid = nt->gid_;
          nghbr.h_view(b,(1+n)/2).lev = nt->loc_.level;
          nghbr.h_view(b,(1+n)/2).rank = ranklist[nt->gid_];
          nghbr.h_view(b,(1+n)/2).destn = (1-n)/2;
        }
      }
    }

    // neighbors on x2face and x1x2 edges
    if (pmy_pack->pmesh->multi_d) {
      for (int m=-1; m<=1; m+=2) {
        MeshBlockTree* nt = ptree->FindNeighbor(loc, 0, m, 0);
        if (nt != nullptr) {
          nghbr.h_view(b,2+(1+m)/2).gid = nt->gid_;
          nghbr.h_view(b,2+(1+m)/2).lev = nt->loc_.level;
          nghbr.h_view(b,2+(1+m)/2).rank = ranklist[nt->gid_];
          nghbr.h_view(b,2+(1+m)/2).destn = 2+(1-m)/2;
        }
      }
      for (int m=-1; m<=1; m+=2) {
        for (int n=-1; n<=1; n+=2) {
          MeshBlockTree* nt = ptree->FindNeighbor(loc, n, m, 0);
          if (nt != nullptr) {
            nghbr.h_view(b,4+(1+m)+(1+n)/2).gid = nt->gid_;
            nghbr.h_view(b,4+(1+m)+(1+n)/2).lev = nt->loc_.level;
            nghbr.h_view(b,4+(1+m)+(1+n)/2).rank = ranklist[nt->gid_];
            nghbr.h_view(b,4+(1+m)+(1+n)/2).destn = 4+(1-m)+(1-n)/2;
          }
        }
      }
    }

    // neighbors on x3face, x3x1 and x2x3 edges, and corners
    if (pmy_pack->pmesh->three_d) {
      for (int l=-1; l<=1; l+=2) {
        MeshBlockTree* nt = ptree->FindNeighbor(loc, 0, 0, l);
        if (nt != nullptr) {
          nghbr.h_view(b,8+(1+l)/2).gid = nt->gid_;
          nghbr.h_view(b,8+(1+l)/2).lev = nt->loc_.level;
          nghbr.h_view(b,8+(1+l)/2).rank = ranklist[nt->gid_];
          nghbr.h_view(b,8+(1+l)/2).destn = 8+(1-l)/2;
        }
      }
      for (int l=-1; l<=1; l+=2) {
        for (int n=-1; n<=1; n+=2) {
          MeshBlockTree* nt = ptree->FindNeighbor(loc, n, 0, l);
          if (nt != nullptr) {
            nghbr.h_view(b,10+(1+l)+(1+n)/2).gid = nt->gid_;
            nghbr.h_view(b,10+(1+l)+(1+n)/2).lev = nt->loc_.level;
            nghbr.h_view(b,10+(1+l)+(1+n)/2).rank = ranklist[nt->gid_];
            nghbr.h_view(b,10+(1+l)+(1+n)/2).destn = 10+(1-l)+(1-n)/2;
          }
        }
      }
      for (int l=-1; l<=1; l+=2) {
        for (int m=-1; m<=1; m+=2) {
          MeshBlockTree* nt = ptree->FindNeighbor(loc, 0, m, l);
          if (nt != nullptr) {
            nghbr.h_view(b,14+(1+l)+(1+m)/2).gid = nt->gid_;
            nghbr.h_view(b,14+(1+l)+(1+m)/2).lev = nt->loc_.level;
            nghbr.h_view(b,14+(1+l)+(1+m)/2).rank = ranklist[nt->gid_];
            nghbr.h_view(b,14+(1+l)+(1+m)/2).destn = 14+(1-l)+(1-m)/2;
          }
        }
      }
      for (int l=-1; l<=1; l+=2) {
        for (int m=-1; m<=1; m+=2) {
          for (int n=-1; n<=1; n+=2) {
            MeshBlockTree* nt = ptree->FindNeighbor(loc, n, m, l);
            if (nt != nullptr) {
              nghbr.h_view(b,18+2*(1+l)+(1+m)+(1+n)/2).gid = nt->gid_;
              nghbr.h_view(b,18+2*(1+l)+(1+m)+(1+n)/2).lev = nt->loc_.level;
              nghbr.h_view(b,18+2*(1+l)+(1+m)+(1+n)/2).rank = ranklist[nt->gid_];
              nghbr.h_view(b,18+2*(1+l)+(1+m)+(1+n)/2).destn = 18+2*(1-l)+(1-m)+(1-n)/2;
            }
          }
        }
      }
    }
  }

  // For each DualArray: mark host views as modified, and then sync to device array
  nghbr.template modify<HostMemSpace>();
  nghbr.template sync<DevExeSpace>();

  return;
}
