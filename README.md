# FHE-Math-For-DL

A C++ implementation of a **Multi-Layer Perceptron (MLP) running entirely under Fully Homomorphic Encryption (FHE)** using the **CKKS** scheme via [OpenFHE](https://github.com/openfheorg/openfhe-development). The model performs neural network inference on encrypted data — the plaintext input is never exposed during computation.

This is a Capstone/CEPP project demonstrating that deep learning inference can be performed on ciphertext with practical accuracy, using the Iris dataset as a benchmark.

---

## Overview

```
Input (encrypted) → Linear(4→8) → Sigmoid → Linear(8→16) → Sigmoid → Linear(16→3) → Output (encrypted)
```

- **Scheme**: CKKS (approximate arithmetic over real numbers)
- **Library**: OpenFHE
- **Task**: 3-class classification on the Iris dataset
- **Activation**: Polynomial approximation of Sigmoid (degree-19 Taylor expansion)
- **Matrix multiplication**: Diagonal (Baby-step Giant-step style) encoding for FHE-efficient linear layers

---

## Project Structure

```
.
├── include/
│   └── fhe_math_dl/
│       ├── mlp.hpp          # MLP class + LinearLayer struct (public API)
│       └── fhe_act.hpp      # FHE activation functions (Sigmoid, SiLU)
├── src/
│   └── mlp.cc               # MLP implementation
├── examples/
│   └── iris_demo.cc         # Full Iris classification demo (entry point)
├── cmake/
│   └── fhe_math_dlConfig.cmake.in  # CMake package config template
├── CMakeLists.txt           # Builds libfhe_math_dl + iris_demo
└── execute.sh               # Helper script: configure, build, and run demo
```

---

## Dependencies

| Dependency                                                   | Purpose                       | Version               |
| ------------------------------------------------------------ | ----------------------------- | --------------------- |
| [OpenFHE](https://github.com/openfheorg/openfhe-development) | CKKS FHE operations           | ≥ 1.1                 |
| [glaze](https://github.com/stephenberry/glaze)               | JSON parsing for weights/data | v2.3.1 (auto-fetched) |
| CMake                                                        | Build system                  | ≥ 3.10                |
| C++ Compiler                                                 | C++17 required                | GCC / Clang           |

> **glaze** is automatically fetched via `FetchContent` during CMake configuration.
> **OpenFHE** must be installed manually (see below).

---

## Prerequisites

### Install OpenFHE

Follow the official [OpenFHE build guide](https://openfhe-development.readthedocs.io/en/latest/sphinx_rsts/intro/installation/installation.html).

On macOS with Homebrew dependencies:
```bash
# Install dependencies
brew install cmake openmp

# Clone and build OpenFHE
git clone https://github.com/openfheorg/openfhe-development.git
cd openfhe-development
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
make -j$(sysctl -n hw.ncpu)
sudo make install
```

### Prepare Data and Weights

The program expects pre-trained MLP weights and test data as JSON files. By default it reads from:

```
/Users/bellian/CEPP/mlp/iris_weights.json   # model weights
/Users/bellian/CEPP/mlp/X_test.json         # test features
/Users/bellian/CEPP/mlp/Y_test.json         # test labels
```

> You can change these paths in `examples/iris_demo.cc` (constants at the top of the file).

**Expected JSON formats:**

`iris_weights.json` — a flat object with keys like `"0.weight"`, `"0.bias"`, `"2.weight"`, etc.:
```json
{
  "0.weight": [[...], [...], ...],
  "0.bias":   [...],
  "2.weight": [[...], ...],
  "2.bias":   [...],
  ...
}
```

`X_test.json`:
```json
{ "data": [[5.1, 3.5, 1.4, 0.2], ...] }
```

`Y_test.json`:
```json
{ "data": [0.0, 1.0, 2.0, ...] }
```

---

## Build & Run

### Using the helper script

```bash
chmod +x execute.sh
./execute.sh
```

This will: configure CMake, clean the `build/` directory, rebuild, and run the binary.

### Manual build

```bash
cmake -B build
cd build
cmake ..
make -j$(sysctl -n hw.ncpu)
sudo -E ./iris_demo
```

> `sudo -E` is used to preserve environment variables (e.g., OpenFHE library paths).

### Install the library

```bash
cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build -j$(sysctl -n hw.ncpu)
cmake --install build
```

This installs headers to `include/fhe_math_dl/` and the static library `libfhe_math_dl.a` to `lib/`.

---

## FHE Parameters

| Parameter            | Value      |
| -------------------- | ---------- |
| Scheme               | CKKS (RNS) |
| Multiplicative Depth | 16         |
| Scaling Mod Size     | 50 bits    |
| Batch Size           | 16 slots   |

---

## How It Works

### Linear Layer (Diagonal Method)

Standard matrix-vector multiplication in FHE is expensive. This project uses the **diagonal encoding** trick:

- The weight matrix is decomposed into diagonals.
- Each diagonal is encoded as a plaintext vector.
- The input ciphertext is rotated and multiplied by the corresponding diagonal, then accumulated.
- This reduces rotations and ciphertext-plaintext multiplications significantly.

### Activation Function (Polynomial Sigmoid)

ReLU is not FHE-friendly (it's not a polynomial). Instead, the sigmoid function `σ(x) = 1 / (1 + e^−x)` is approximated by a **degree-19 odd Taylor polynomial** around 0, using precomputed coefficients stored in `SIGMOID_COEFFS`.

Powers of the ciphertext are computed efficiently using a binary-exponentiation-style strategy over even powers.

**SiLU** (`x · σ(x)`) is also available in `include/fhe_math_dl/fhe_act.hpp` as an alternative.

---

## Output

The program prints per-sample predictions, then a summary:

```
sample 0 pred: 0 label: 0 ✓
sample 1 pred: 1 label: 1 ✓
...

=== Confusion Matrix ===
          pred:0  pred:1  pred:2
actual:0       X       X       X
actual:1       X       X       X
actual:2       X       X       X

=== Per Class ===
   class precision    recall
       0     100.0%     100.0%
       1      92.3%      85.7%
       2      ...

Total:     30
Skipped:   0
Evaluated: 30
Accuracy:  28/30 = 93.33%
Time: 42.5 seconds
```

---

## Using as a Library

After installing, consume `fhe_math_dl` from another CMake project:

```cmake
find_package(OpenFHE REQUIRED PATHS /usr/local/lib/OpenFHE)
find_package(fhe_math_dl REQUIRED)

target_link_libraries(my_project PRIVATE fhe_math_dl::fhe_math_dl)
```

Then include the headers:

```cpp
#include "fhe_math_dl/mlp.hpp"
#include "fhe_math_dl/fhe_act.hpp"
```

---

## Limitations & Notes

- **Performance**: FHE inference is orders of magnitude slower than plaintext inference. Each sample takes several seconds.
- **Precision**: CKKS is approximate; some samples may throw exceptions due to noise accumulation at high multiplicative depth and are counted as "skipped".
- **Batch size**: The CKKS batch size is 16 slots (powers-of-two requirement), limiting parallelism within a single ciphertext.
- **Hardcoded paths**: Data/weight file paths are currently hardcoded in `examples/iris_demo.cc`.
