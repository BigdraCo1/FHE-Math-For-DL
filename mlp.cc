#include "mlp.hpp"

void MLP::add_layer(
    const std::vector<std::vector<double>>& W,
    const std::vector<double>& b) {

    layers.push_back({
        .weights = W,
        .bias    = b,
        .in_dim  = (int)W[0].size(),  // cols
        .out_dim = (int)W.size()       // rows
    });
}

Ciphertext<DCRTPoly> MLP::linear_layer(
    const Ciphertext<DCRTPoly>& ct_input,
    const LinearLayer& layer,
    const CryptoContext<DCRTPoly>& cc
) {
    // precompute diagonals
    std::vector<std::vector<double>> diags(layer.in_dim, std::vector<double>(layer.out_dim));       // z0 -> z1 -> ... -> z_{in_dim-1}
    for (int k = 0; k < layer.in_dim; k++) {
        for (int i = 0; i < layer.out_dim; i++) {
            diags[k][i] = layer.weights[i][(i + k) % layer.in_dim];
        }
    }

    std::cout << "DIAGONALS COMPUTED" << std::endl;

    // accumulate
    Ciphertext<DCRTPoly> ct_result;
    bool first = true;

    for (int k = 0; k < layer.in_dim; k++) {
        auto ct_rot = (k == 0) ? ct_input : cc->EvalRotate(ct_input, k);
        std::cout << "ROTATED " << k << std::endl;
        auto ct_mul  = cc->EvalMult(ct_rot,                                             // z_k ⊕ rot(input, k)
                           cc->MakeCKKSPackedPlaintext(diags[k]));
        if (first) { ct_result = ct_mul; first = false; }
        else         ct_result = cc->EvalAdd(ct_result, ct_mul);
    }

    // add bias
    Plaintext pt_bias = cc->MakeCKKSPackedPlaintext(layer.bias);
    ct_result = cc->EvalAdd(ct_result, pt_bias);
    return ct_result;
}

void MLP::load_weights(const std::string& filename) {
    std::map<std::string, std::vector<std::vector<double>>> wMap;
    std::map<std::string, std::vector<double>> bMap;
    glz::json_t json{};
    std::string buffer{};
    auto error = glz::read_file_json(json, filename, buffer);
    if (error) {
        std::cerr << glz::format_error(error, buffer) << std::endl;
        return;
    }

    auto& obj = json.get<glz::json_t::object_t>();
    for (auto& [key, value] : obj) {
        if (!value.holds<glz::json_t::array_t>()) continue;

        auto& arr = value.get<glz::json_t::array_t>();

        // 2D (weights)
        if (!arr.empty() && arr[0].holds<glz::json_t::array_t>()) {
            auto& wmat = wMap[key];
            wmat.resize(arr.size());
            for (size_t i = 0; i < arr.size(); ++i) {
                auto& row = arr[i].get<glz::json_t::array_t>();  // cache reference
                wmat[i].resize(row.size());
                for (size_t j = 0; j < row.size(); ++j) {
                    wmat[i][j] = row[j].get<double>();
                }
            }
        }
        // 1D (biases)
        else {
            auto& bvec = bMap[key];
            bvec.resize(arr.size());
            for (size_t i = 0; i < arr.size(); ++i) {
                bvec[i] = arr[i].get<double>();
            }
        }
    }

    for (auto& [key, value] : wMap) {
        std::string bias_key = key.substr(0, key.find_last_of('.')) + ".bias";
        add_layer(value, bMap[bias_key]);
    }
}

Ciphertext<DCRTPoly> MLP::forward(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc) {
    Ciphertext<DCRTPoly> ct = input;
    std::cout << "FORWARDING STARTED"<< std::endl;
    ct = linear_layer(ct, layers[0], cc);
    std::cout << "LAYER 1 COMPLETED" << std::endl;
    ct = polynomial_sigmoid_fhe(ct, cc, SIGMOID_COEFFS);
    ct = linear_layer(ct, layers[1], cc);
    std::cout << "LAYER 2 COMPLETED" << std::endl;
    ct = polynomial_sigmoid_fhe(ct, cc, SIGMOID_COEFFS);
    ct = linear_layer(ct, layers[2], cc);
    return ct;
}

Ciphertext<DCRTPoly> MLP::fwd_linear1(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc) {
    Ciphertext<DCRTPoly> ct = input;
    ct = linear_layer(ct, layers[0], cc);
    std::cout << "LAYER 1 COMPLETED" << std::endl;
    return ct;
}

Ciphertext<DCRTPoly> MLP::fwd_sigmoid1(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc) {
    Ciphertext<DCRTPoly> ct = input;
    ct = polynomial_sigmoid_fhe(ct, cc, SIGMOID_COEFFS);
    std::cout << "SIGMOID 1 COMPLETED" << std::endl;
    return ct;
}

Ciphertext<DCRTPoly> MLP::fwd_linear2(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc) {
    Ciphertext<DCRTPoly> ct = input;
    ct = linear_layer(ct, layers[1], cc);
    std::cout << "LAYER 2 COMPLETED" << std::endl;
    return ct;
}

Ciphertext<DCRTPoly> MLP::fwd_sigmoid2(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc) {
    Ciphertext<DCRTPoly> ct = input;
    ct = polynomial_sigmoid_fhe(ct, cc, SIGMOID_COEFFS);
    std::cout << "SIGMOID 2 COMPLETED" << std::endl;
    return ct;
}

Ciphertext<DCRTPoly> MLP::fwd_linear3(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc) {
    Ciphertext<DCRTPoly> ct = input;
    ct = linear_layer(ct, layers[2], cc);
    std::cout << "LAYER 3 COMPLETED" << std::endl;
    return ct;
}
