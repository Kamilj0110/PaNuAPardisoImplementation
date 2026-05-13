#include "pardiso_solver.h"

#include <iostream>
#include <vector>
#include <cmath>

// ----------------------------------------------------------------------------
// Panua Pardiso interface (Fortran-style C library)
// ----------------------------------------------------------------------------
extern "C" {
    void pardisoinit(void* pt, int* mtype, int* solver, int* iparm, double* dparm, int* error);
    void pardiso(void* pt, int* maxfct, int* mnum, int* mtype, int* phase,
                 int* n, double* a, int* ia, int* ja, int* perm, int* nrhs,
                 int* iparm, int* msglvl, double* b, double* x, int* error, double* dparm);
}

// ----------------------------------------------------------------------------
// Internal: Pardiso-compatible CSR (1-based indexing, 32-bit int)
// Pardiso requires int (not int64_t) and 1-based indices.
// ----------------------------------------------------------------------------
struct PardisoCSR {
    int n;
    std::vector<int> ia;   // row pointers   (1-based, size n+1)
    std::vector<int> ja;   // column indices (1-based, size nnz)
    std::vector<double> a; // values         (size nnz)
};

// ----------------------------------------------------------------------------
// Internal: Convert colleague's CSR (0-based int64_t) -> PardisoCSR (1-based int)
// ----------------------------------------------------------------------------
static PardisoCSR to_pardiso_csr(const SparseMatrixCSR& csr) {
    PardisoCSR p;
    p.n = static_cast<int>(csr.nrows);

    // Row pointers: 0-based int64_t -> 1-based int
    p.ia.resize(csr.nrows + 1);
    for (int i = 0; i <= p.n; i++)
        p.ia[i] = static_cast<int>(csr.rowOffsets[i]) + 1;

    // Column indices: 0-based int64_t -> 1-based int
    p.ja.resize(csr.nnz);
    for (int64_t i = 0; i < csr.nnz; i++)
        p.ja[i] = static_cast<int>(csr.colIndices[i]) + 1;

    // Values: direct copy
    p.a = csr.values;

    return p;
}

// ----------------------------------------------------------------------------
// Internal: Compute residual norm ||b - Ax|| for verification
// ----------------------------------------------------------------------------
static double compute_residual(const PardisoCSR& p,
                               const std::vector<double>& b,
                               const std::vector<double>& x) {
    double res = 0.0;
    for (int i = 0; i < p.n; i++) {
        double val = b[i];
        for (int j = p.ia[i] - 1; j < p.ia[i + 1] - 1; j++)
            val -= p.a[j] * x[p.ja[j] - 1];
        res += val * val;
    }
    return std::sqrt(res);
}

// ----------------------------------------------------------------------------
// Public: solve Ax = b using Panua Pardiso
// ----------------------------------------------------------------------------
bool solveWithPardiso(
    const SparseMatrixCSR& csr,
    const std::vector<double>& b,
    std::vector<double>& x)
{
    // --- Convert to Pardiso format ---
    PardisoCSR p = to_pardiso_csr(csr);
    x.assign(p.n, 0.0);

    // --- Pardiso internal state ---
    void*  pt[64]    = {};
    int    iparm[64] = {};
    double dparm[64] = {};

    // mtype 11 = real unsymmetric (safe general default)
    // Change to 3 for symmetric positive definite (better performance if applicable)
    int mtype  = 11;
    int solver = 0;   // sparse direct solver
    int error  = 0;

    pardisoinit(pt, &mtype, &solver, iparm, dparm, &error);
    if (error != 0) {
        std::cerr << "[Pardiso] Initialization error: " << error << std::endl;
        return false;
    }

    iparm[0]  = 1;   // manual parameter setting
    iparm[1]  = 3;   // METIS fill-reducing reordering
    iparm[7]  = 2;   // max iterative refinement steps
    iparm[9]  = 13;  // pivot perturbation (10^-13)
    iparm[10] = 1;   // enable scaling
    iparm[12] = 1;   // enable matching
    iparm[17] = -1;  // report nnz in factors
    iparm[18] = -1;  // report Mflops for factorization
    iparm[34] = 0;   // 1-based indexing

    int maxfct = 1, mnum = 1, nrhs = 1, msglvl = 1;
    int n = p.n;
    std::vector<int> perm(n, 0);

    // --- Phase 11: Reordering + symbolic factorization ---
    int phase = 11;
    pardiso(pt, &maxfct, &mnum, &mtype, &phase, &n,
            p.a.data(), p.ia.data(), p.ja.data(),
            perm.data(), &nrhs, iparm, &msglvl,
            const_cast<double*>(b.data()), x.data(), &error, dparm);
    if (error != 0) {
        std::cerr << "[Pardiso] Phase 11 error: " << error << std::endl;
        return false;
    }

    // --- Phase 22: Numerical factorization ---
    phase = 22;
    pardiso(pt, &maxfct, &mnum, &mtype, &phase, &n,
            p.a.data(), p.ia.data(), p.ja.data(),
            perm.data(), &nrhs, iparm, &msglvl,
            const_cast<double*>(b.data()), x.data(), &error, dparm);
    if (error != 0) {
        std::cerr << "[Pardiso] Phase 22 error: " << error << std::endl;
        return false;
    }

    // --- Phase 33: Back substitution + iterative refinement ---
    phase = 33;
    pardiso(pt, &maxfct, &mnum, &mtype, &phase, &n,
            p.a.data(), p.ia.data(), p.ja.data(),
            perm.data(), &nrhs, iparm, &msglvl,
            const_cast<double*>(b.data()), x.data(), &error, dparm);
    if (error != 0) {
        std::cerr << "[Pardiso] Phase 33 error: " << error << std::endl;
        return false;
    }

    // --- Verify ---
    double residual = compute_residual(p, b, x);
    std::cout << "[Pardiso] Residual norm ||b - Ax|| = " << residual << std::endl;
    if (residual > 1e-6)
        std::cerr << "[Pardiso] Warning: residual is large -- check matrix type (mtype)" << std::endl;

    // --- Release internal memory ---
    phase = -1;
    pardiso(pt, &maxfct, &mnum, &mtype, &phase, &n,
            p.a.data(), p.ia.data(), p.ja.data(),
            perm.data(), &nrhs, iparm, &msglvl,
            const_cast<double*>(b.data()), x.data(), &error, dparm);

    return true;
}
