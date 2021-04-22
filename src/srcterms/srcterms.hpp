#ifndef SRCTERMS_SRCTERMS_HPP_
#define SRCTERMS_SRCTERMS_HPP_
//========================================================================================
// AthenaXXX astrophysical plasma code
// Copyright(C) 2020 James M. Stone <jmstone@ias.edu> and the Athena code team
// Licensed under the 3-clause BSD License (the "LICENSE")
//========================================================================================
//! \file srcterms.hpp
//! \brief Data, functions, and classes to implement various source terms in the hydro 
//! and/or MHD equations of motion.  Currently implemented:
//!  (1) constant (gravitational) acceleration - for RTI
//!  (2) shearing box in 2D (x-z), for both hydro and MHD
//!  (3) random forcing to drive turbulence - implemented in TurbulenceDriver class

#include <map>
#include "athena.hpp"
#include "parameter_input.hpp"
#include "mesh/mesh.hpp"
#include "eos/eos.hpp"

// forward declarations
class TurbulenceDriver;
class Driver;

//----------------------------------------------------------------------------------------
//! \class SourceTerms
//! \brief data and functions for physical source terms

class SourceTerms
{
 public:
  SourceTerms(std::string block, MeshBlockPack *pp, ParameterInput *pin);
  ~SourceTerms();

  // data
  // flags for various source terms
  bool const_accel;
  bool shearing_box;

  // constants/coefficients for various terms
  Real const_accel_val;
  int  const_accel_dir;
  Real omega0, qshear;

  // functions
  void AddConstantAccel(DvceArray5D<Real> &u0,const DvceArray5D<Real> &w0,const Real dt);
  void AddShearingBox(DvceArray5D<Real> &u0,const DvceArray5D<Real> &w0,const Real dt);
  void AddShearingBox(DvceArray5D<Real> &u0, const DvceArray5D<Real> &w0,
                      const DvceArray5D<Real> &bcc, const Real dt);
  void AddSBoxEField(const DvceFaceFld4D<Real> &b0, DvceEdgeFld4D<Real> &efld);

 private:
  MeshBlockPack* pmy_pack;
};

#endif // SRCTERMS_SRCTERMS_HPP_
