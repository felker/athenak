//========================================================================================
// AthenaXXX astrophysical plasma code
// Copyright(C) 2020 James M. Stone <jmstone@ias.edu> and the Athena code team
// Licensed under the 3-clause BSD License (the "LICENSE")
//========================================================================================
//! \file radiation_femn_linetest.cpp
//! \brief initializes the 2d linetest problem with FEM_N/FP_N/M1

// C++ headers
#include <iostream>
#include <math.h>        // exp

// AthenaK headers
#include "athena.hpp"
#include "parameter_input.hpp"
#include "coordinates/cell_locations.hpp"
#include "mesh/mesh.hpp"
#include "pgen/pgen.hpp"
#include "radiation_femn/radiation_femn.hpp"
#include "adm/adm.hpp"

void ProblemGenerator::RadiationFEMNLinetest(ParameterInput* pin, const bool restart)
{
    if (restart) return;

    MeshBlockPack* pmbp = pmy_mesh_->pmb_pack;

    if (pmbp->pradfemn == nullptr)
    {
        std::cout << "### FATAL ERROR in " << __FILE__ << " at line " << __LINE__ << std::endl
            << "The 2d line source problem generator can only be run with radiation-femn, but no "
            << "<radiation-femn> block in input file" << std::endl;
        exit(EXIT_FAILURE);
    }

    if (!pmbp->pmesh->two_d)
    {
        std::cout << "### FATAL ERROR in " << __FILE__ << " at line " << __LINE__ << std::endl
            << "The 2d line source problem generator can only be run with two dimensions, but parfile"
            << "grid setup is not in 2d" << std::endl;
        exit(EXIT_FAILURE);
    }

    auto& indcs = pmy_mesh_->mb_indcs;
    auto& size = pmbp->pmb->mb_size;
    int& is = indcs.is;
    int& ie = indcs.ie;
    int& js = indcs.js;
    int& je = indcs.je;
    int& ks = indcs.ks;
    int& ke = indcs.ke;
    int npts1 = pmbp->pradfemn->num_points_total - 1;

    int isg = is - indcs.ng;
    int ieg = ie + indcs.ng;
    int jsg = (indcs.nx2 > 1) ? js - indcs.ng : js;
    int jeg = (indcs.nx2 > 1) ? je + indcs.ng : je;
    int ksg = (indcs.nx3 > 1) ? ks - indcs.ng : ks;
    int keg = (indcs.nx3 > 1) ? ke + indcs.ng : ke;
    int nmb = pmbp->nmb_thispack;
    int &num_energy_bins_ = pmbp->pradfemn->num_energy_bins;
    int &num_points_ = pmbp->pradfemn->num_points;
    auto& u_mu_ = pmbp->pradfemn->u_mu;
    adm::ADM::ADM_vars& adm = pmbp->padm->adm;

    Real omega = 0.03; //  Eqn. (58) of Garrett & Hauck 2013 (DOI: 10.1080/00411450.2014.910226)
    auto& f0_ = pmbp->pradfemn->f0;

    if (!pmbp->pradfemn->fpn)
    {
        par_for("pgen_linetest_radiation_femn", DevExeSpace(), 0, (pmbp->nmb_thispack - 1), 0, npts1, ks, ke, js, je, is, ie,
                KOKKOS_LAMBDA(int m, int A, int k, int j, int i)
                {
                    Real& x1min = size.d_view(m).x1min;
                    Real& x1max = size.d_view(m).x1max;
                    int nx1 = indcs.nx1;
                    Real x1 = CellCenterX(i - is, nx1, x1min, x1max);

                    Real& x2min = size.d_view(m).x2min;
                    Real& x2max = size.d_view(m).x2max;
                    int nx2 = indcs.nx2;
                    Real x2 = CellCenterX(j - js, nx2, x2min, x2max);

                    f0_(m, A, k, j, i) = fmax(exp(-(x1 * x1 + x2 * x2) / (2.0 * omega * omega)) / (8.0 * M_PI * omega * omega), 1e-4);
                });
    }
    else
    {
        par_for("pgen_linetest_radiation_fpn", DevExeSpace(), 0, (pmbp->nmb_thispack - 1), ks, ke, js, je, is, ie,
                KOKKOS_LAMBDA(int m, int k, int j, int i)
                {
                    Real& x1min = size.d_view(m).x1min;
                    Real& x1max = size.d_view(m).x1max;
                    int nx1 = indcs.nx1;
                    Real x1 = CellCenterX(i - is, nx1, x1min, x1max);

                    Real& x2min = size.d_view(m).x2min;
                    Real& x2max = size.d_view(m).x2max;
                    int nx2 = indcs.nx2;
                    Real x2 = CellCenterX(j - js, nx2, x2min, x2max);

                    for (int en = 0; en < num_energy_bins_; en++) {
                      f0_(m, en * num_points_, k, j, i) = 2. * sqrt(M_PI) * fmax(
                          exp(-(x1 * x1 + x2 * x2) / (2.0 * omega * omega))
                              / (8.0 * M_PI * omega * omega), 1e-4);
                    }
                });
    }

    // set metric to minkowski
    par_for("pgen_linetest_metric_initialize", DevExeSpace(), 0, nmb - 1, ksg, keg, jsg, jeg, isg, ieg,
            KOKKOS_LAMBDA(const int m, const int k, const int j, const int i)
            {
                for (int a = 0; a < 3; ++a)
                    for (int b = a; b < 3; ++b)
                    {
                        adm.g_dd(m, a, b, k, j, i) = (a == b ? 1. : 0.);
                    }

                adm.psi4(m, k, j, i) = 1.; // adm.psi4

                adm.alpha(m, k, j, i) = 1.;

                u_mu_(m, 0, k, j, i) = 1.;
                u_mu_(m, 1, k, j, i) = 0.;
                u_mu_(m, 2, k, j, i) = 0.;
                u_mu_(m, 3, k, j, i) = 0.;
            });
}
