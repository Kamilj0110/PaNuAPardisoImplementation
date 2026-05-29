# Pardiso Sparse Solver

Solves sparse linear systems `Ax = b` using **Panua Pardiso**.
Reads matrices in our custom binary COO format.

## Prerequisites

1. **Panua Pardiso** — get an license at https://panua.ch
2. Place your `panua.lic` in your home directory:
   - Windows: `C:\Users\YourName\panua.lic`
   - Linux: `~/panua.lic`
3. **CMake** ≥ 3.20
4. A C++17 compiler (MSVC, GCC, or Clang)

> ⚠️ The Pardiso DLLs/.so files are **not included** in this repo.
> You must download them from Panua and place them in your local install folder.

## Building (Windows)

```cmd
cmake -B build -G "Ninja" -DPARDISO_DIR=C:/path/to/panua-pardiso-YYYYMMDD-win -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Building (Linux)

```bash
sudo apt install build-essential cmake gfortran libomp-dev
cmake -B build -DPARDISO_DIR=$HOME/panua-pardiso -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## Running

```cmd
set OMP_NUM_THREADS=4
build\pardiso_solver.exe matrix_A.bin vector_b.bin
```

## Project structure

| File | Purpose |
|---|---|
| `main.cpp` | Orchestration |
| `matrix_loader.h/.cpp` | Binary COO file I/O + COO→CSR conversion |
| `pardiso_solver.h/.cpp` | Pardiso wrapper (handles 0→1-based indexing) |
| `CMakeLists.txt` | Cross-platform build configuration |

## Binary file format

See `docs/Binary_Linear_System_Format.pdf` for details.

- Files: `<name>_A.bin` (matrix) and `<name>_b.bin` (vector)
- Indexing: 0-based, `uint64`
- Values: `double`
