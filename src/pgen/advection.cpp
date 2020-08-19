//========================================================================================
// AthenaXXX astrophysical plasma code
// Copyright(C) 2020 James M. Stone <jmstone@ias.edu> and the Athena code team
// Licensed under the 3-clause BSD License (the "LICENSE")
//========================================================================================
//! \file advection.c
//  \brief Problem generator for advection problems.  Use with evolve=advect
//
// Input parameters are:
//    - problem/u0   = flow speed

#include <cmath>
#include <iostream>   // endl
#include <sstream>    // stringstream
#include <stdexcept>  // runtime_error
#include <string>     // c_str()

#include "athena.hpp"
#include "athena_arrays.hpp"
#include "parameter_input.hpp"
#include "mesh/mesh.hpp"
#include "hydro/hydro.hpp"
#include "pgen.hpp"

//----------------------------------------------------------------------------------------
//! \fn void MeshBlock::ProblemGenerator(ParameterInput *pin)
//  \brief Problem Generator for advection problems

void ProblemGenerator::Advection_(MeshBlock *pmb, std::unique_ptr<ParameterInput> &pin) {
using namespace hydro;

  // Read input parameters
  int flow_dir = pin->GetInteger("problem","flow_dir");
  int iprob = pin->GetInteger("problem","iproblem");
  Real vel = pin->GetOrAddReal("problem","velocity",1.0);
  Real amp = pin->GetOrAddReal("problem","amplitude",0.1);

  // Initialize the grid
  int &is = pmb->mb_cells.is, &ie = pmb->mb_cells.ie;
  int &js = pmb->mb_cells.js, &je = pmb->mb_cells.je;
  int &ks = pmb->mb_cells.ks, &ke = pmb->mb_cells.ke;

  // get size of overall domain
  Real length;
  if (flow_dir == 1) {
    length = pmb->pmesh_mb->mesh_size.x1max - pmb->pmesh_mb->mesh_size.x1min;
  } else if (flow_dir == 2) {
    length = pmb->pmesh_mb->mesh_size.x2max - pmb->pmesh_mb->mesh_size.x2min;
  } else if (flow_dir == 3) {
    length = pmb->pmesh_mb->mesh_size.x3max - pmb->pmesh_mb->mesh_size.x3min;
  } else {
    std::cout << "### FATAL ERROR in " << __FILE__ << " at line " << __LINE__ << std::endl
              << "flow_dir=" << flow_dir << " must be either 1,2, or 3" << std::endl;
    exit(EXIT_FAILURE);
  }

  Real &x1min = pmb->mb_size.x1min, &x1max = pmb->mb_size.x1max;
  Real &x2min = pmb->mb_size.x2min, &x2max = pmb->mb_size.x2max;
  Real &x3min = pmb->mb_size.x3min, &x3max = pmb->mb_size.x3max;
  for (int k=ks; k<=ke; k++) {
    for (int j=js; j<=je; j++) {
      for (int i=is-pmb->mb_cells.ng; i<=ie+pmb->mb_cells.ng; i++) {
        Real r; // coordinate that will span [0->1]
        if (flow_dir == 1) {
          r = (pmb->pmesh_mb->CellCenterX(i, pmb->mb_cells.nx1,x1min,x1max)-x1min)/length;
        } else if (flow_dir == 2) {
          r = (pmb->pmesh_mb->CellCenterX(j, pmb->mb_cells.nx2,x2min,x2max)-x2min)/length;
        } else {
          r = (pmb->pmesh_mb->CellCenterX(k, pmb->mb_cells.nx3,x3min,x3max)-x3min)/length;
        }

        // iprob=1: sine wave
        if (iprob == 1) {
          pmb->phydro->u0(IDN,k,j,i) = 1.0 + amp*std::sin(2.0*(M_PI)*r);

        // iprob=2: square wave in second quarter of domain
        } else if (iprob == 2) {
          pmb->phydro->u0(IDN,k,j,i) = 1.0;
          if (r >= 0.25 && r <= 0.5) pmb->phydro->u0(IDN,k,j,i) += amp;
        } else {
          std::cout << "### FATAL ERROR in " << __FILE__ << " at line " << __LINE__
             << std::endl << "problem/iproblem=" << iprob 
             << " not supported" << std::endl;
          exit(EXIT_FAILURE);
        }

        // now compute momenta, total energy
        if (flow_dir == 1) {
          pmb->phydro->u0(IM1,k,j,i) = vel*pmb->phydro->u0(IDN,k,j,i);
          pmb->phydro->u0(IM2,k,j,i) = 0.0;
          pmb->phydro->u0(IM3,k,j,i) = 0.0;
        } else if (flow_dir == 2) {
          pmb->phydro->u0(IM1,k,j,i) = 0.0;
          pmb->phydro->u0(IM2,k,j,i) = vel*pmb->phydro->u0(IDN,k,j,i);
          pmb->phydro->u0(IM3,k,j,i) = 0.0;
        } else {
          pmb->phydro->u0(IM1,k,j,i) = 0.0;
          pmb->phydro->u0(IM2,k,j,i) = 0.0;
          pmb->phydro->u0(IM3,k,j,i) = vel*pmb->phydro->u0(IDN,k,j,i);
        } 
        
        pmb->phydro->u0(IEN,k,j,i) = 1.0;
      }
    }
  }

  return;
}
