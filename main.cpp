#include <iostream>
#include <vector>
#include <string>

#include "matrix_loader.h"
#include "pardiso_solver.h"

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0]
                  << " <matrix_A.bin> <vector_b.bin>" << std::endl;
        return 1;
    }

    const std::string matrixFile = argv[1];
    const std::string rhsFile    = argv[2];

    // -------------------------------------------------------------------------
    // 1. Load matrix and RHS vector from binary files (professor's format)
    // -------------------------------------------------------------------------
    SparseMatrixCOO coo;
    if (!loadMatrixCOO(matrixFile, coo)) return 1;

    std::vector<double> b;
    if (!loadVector(rhsFile, b)) return 1;

    // -------------------------------------------------------------------------
    // 2. Convert COO -> CSR
    // -------------------------------------------------------------------------
    SparseMatrixCSR csr;
    convertCOOtoCSR(coo, csr);

    // -------------------------------------------------------------------------
    // 3. Solve Ax = b with Panua Pardiso
    // -------------------------------------------------------------------------
    std::vector<double> x;
    if (!solveWithPardiso(csr, b, x)) return 1;

    // -------------------------------------------------------------------------
    // 4. Print first 10 solution values
    // -------------------------------------------------------------------------
    int print_n = static_cast<int>(std::min<size_t>(x.size(), 10));
    std::cout << "\nFirst " << print_n << " solution values:" << std::endl;
    for (int i = 0; i < print_n; i++)
        std::cout << "  x[" << i << "] = " << x[i] << std::endl;
    if (x.size() > 10) std::cout << "  ..." << std::endl;

    return 0;
}
