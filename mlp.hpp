#pragma once

#include "openfhe.h"
#include "glaze/glaze.hpp"
#include <vector>
#include <map>
#include <string>
#include "fhe_act.hpp"

using namespace lbcrypto;

struct LinearLayer {
    std::vector<std::vector<double>> weights; // shape (out, in)
    std::vector<double> bias;
    int in_dim;
    int out_dim;
};

class MLP {
public:
    MLP() = default;

    void load_weights(const std::string& filename);

    Ciphertext<DCRTPoly> forward(
        const Ciphertext<DCRTPoly>& input,
        const CryptoContext<DCRTPoly>& cc
    );

private:
    std::vector<LinearLayer> layers;

    Ciphertext<DCRTPoly> linear_layer(
        const Ciphertext<DCRTPoly>& ct_input,
        const LinearLayer& layer,
        const CryptoContext<DCRTPoly>& cc,
        bool repeat_input
    );

    void add_layer(                                  // internal only
            const std::vector<std::vector<double>>& W,
            const std::vector<double>& b);

    Ciphertext<DCRTPoly> linear(
        const Ciphertext<DCRTPoly>& input,
        const LinearLayer& layer,
        CryptoContext<DCRTPoly> cc
    );

    Ciphertext<DCRTPoly> sigmoid(
        const Ciphertext<DCRTPoly>& input,
        CryptoContext<DCRTPoly> cc
    );
};
