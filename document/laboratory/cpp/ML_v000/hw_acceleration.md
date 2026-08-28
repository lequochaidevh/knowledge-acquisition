Hardware Acceleration Module

### Method 1:
Multi-core CPU Parallelization via OpenMP: By injecting a single compiler directive layout (#pragma omp parallel for), C++ forces all your available CPU logical cores to calculate matrix elements concurrently. This yields an instant 4x-8x execution speedup without messing with your core structures.

```cpp
#pragma omp parallel for collapse(2)
```

```sh
g++ -O3 -fopenmp train.cpp -o train
```

### Method 2:
Vectorization via SIMD (AVX2): Uses specialized CPU intrinsics hardware registers to calculate 8 float multiplications simultaneously inside a single clock cycle rather than parsing single elements sequentially.