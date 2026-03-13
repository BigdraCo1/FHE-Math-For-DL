#pragma once
#include "openfhe.h"
#include <vector>
#include <array>
#include <map>

using namespace lbcrypto;

static constexpr std::array<double,11> SIGMOID_COEFFS = {
    0.5,                                        // σ(0)
    0.25,                                       // σ'(0)
    -0.020833333333333332,                      // σ'''(0)
    0.0020833333333333333,                      // σ(5)(0)
    -0.00021081349206349207,                    // σ(7)(0)
    2.1356922398589065e-05,                     // σ(9)(0)
    -2.1638758617925286e-06,                    // σ(11)(0)
    2.192460959822071e-07,                      // σ(13)(0)
    -2.2214269821950048e-08,                    // σ(15)(0)
    2.250776065618843e-09,                      // σ(17)(0)
    -2.2805129455905197e-10                     // σ(19)(0)
};

inline int log2_int(unsigned int n) {
    return n ? 31 - __builtin_clz(n) : -1;
}

////////////SIGMOID////////////
// σ(x) = 1 / (1 + exp(-x)) //
/////////////////////////////
template <size_t N>
Ciphertext<DCRTPoly> polynomial_sigmoid_fhe(
    const Ciphertext<DCRTPoly>& x,
    const CryptoContext<DCRTPoly>& cc,
    const std::array<double, N>& coeffs,
    int valid_slots = -1
) {
    int batch = (int)cc->GetEncodingParams()->GetBatchSize();

    Ciphertext<DCRTPoly> ct_in = x;
    if (valid_slots > 0 && valid_slots < batch) {
        for (int offset = valid_slots; offset < batch; offset += valid_slots) {
            int rot = -offset;
            if (rot < -(batch/2)) rot += batch;
            ct_in = cc->EvalAdd(ct_in, cc->EvalRotate(x, rot));
        }
    }

    int max_deg    = 2 * coeffs.size() - 3;
    int num_levels = log2_int(max_deg);
    std::vector<Ciphertext<DCRTPoly>> evenPow(num_levels);
    evenPow[0] = cc->EvalMult(ct_in, ct_in);
    for (int k = 1; k < num_levels; k++)
        evenPow[k] = cc->EvalMult(evenPow[k-1], evenPow[k-1]);

    std::map<int, Ciphertext<DCRTPoly>> powMap;
    powMap[1] = ct_in;
    for (int p = 3; p <= max_deg; p += 2) {
        int k   = log2_int(p);
        int rem = p - (1 << k);
        powMap[p] = (rem == 1)
            ? cc->EvalMult(evenPow[k-1], ct_in)
            : cc->EvalMult(evenPow[k-1], powMap[rem]);
    }

    auto result = cc->EvalMult(ct_in, coeffs[1]);
    result      = cc->EvalAdd(result, coeffs[0]);
    for (int i = 2; i < (int)coeffs.size(); i++) {
        int deg = 2 * i - 1;
        result  = cc->EvalAdd(result, cc->EvalMult(powMap[deg], coeffs[i]));
    }
    return result;
}

////////////SILU///////////////
//        x * σ(x)          //
/////////////////////////////
template <size_t N>
Ciphertext<DCRTPoly> silu_fhe(
    const Ciphertext<DCRTPoly>& x,
    const CryptoContext<DCRTPoly>& cc,
    const std::array<double, N>& coeffs,
    int valid_slots = -1
) {
    return cc->EvalMult(x, polynomial_sigmoid_fhe(x, cc, coeffs, valid_slots));
}
