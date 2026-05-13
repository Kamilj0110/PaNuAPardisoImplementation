#pragma once

#include <vector>
#include "matrix_loader.h"

// Converts colleague's CSR (0-based, int64_t) to Pardiso-compatible CSR
// (1-based, int) and solves Ax = b using Panua Pardiso.
//
// Parameters:
//   csr  - CSR matrix from convertCOOtoCSR()
//   b    - right-hand side vector
//   x    - output solution vector (will be resized and filled)
//
// Returns true if the solve succeeded, false otherwise.
bool solveWithPardiso(
    const SparseMatrixCSR& csr,
    const std::vector<double>& b,
    std::vector<double>& x);
