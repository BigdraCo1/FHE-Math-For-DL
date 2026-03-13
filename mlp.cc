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
    const CryptoContext<DCRTPoly>& cc,
    bool repeat_input = false
) {
    int batch = (int)cc->GetEncodingParams()->GetBatchSize();

    Ciphertext<DCRTPoly> ct_in = ct_input;
    if (repeat_input) {
        for (int offset = layer.in_dim; offset < batch; offset += layer.in_dim) {
            int rot = -offset;
            if (rot < -(batch/2)) rot += batch;
            ct_in = cc->EvalAdd(ct_in, cc->EvalRotate(ct_input, rot));
        }
    }

    std::vector<std::vector<double>> diags(layer.in_dim, std::vector<double>(layer.out_dim));
    for (int k = 0; k < layer.in_dim; k++)
        for (int i = 0; i < layer.out_dim; i++)
            diags[k][i] = layer.weights[i][(i + k) % layer.in_dim];

    Ciphertext<DCRTPoly> ct_result;
    bool first = true;
    for (int k = 0; k < layer.in_dim; k++) {
        int rot_k = k;
        if (rot_k > batch / 2) rot_k -= batch;
        auto ct_rot = (k == 0) ? ct_in : cc->EvalRotate(ct_in, rot_k);
        auto ct_mul = cc->EvalMult(ct_rot, cc->MakeCKKSPackedPlaintext(diags[k]));
        if (first) { ct_result = ct_mul; first = false; }
        else         ct_result = cc->EvalAdd(ct_result, ct_mul);
    }

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
    // Linear(4,8), Linear(8,16), Linear(16,3)
    Ciphertext<DCRTPoly> ct = input;
    std::cout << "FORWARDING STARTED"<< std::endl;
    ct = linear_layer(ct, layers[0], cc, true);
    std::cout << "LAYER 1 COMPLETED" << std::endl;
    ct = polynomial_sigmoid_fhe(ct, cc, SIGMOID_COEFFS, 8);
    std::cout << "SIGMOID 1 COMPLETED" << std::endl;
    ct = linear_layer(ct, layers[1], cc);
    std::cout << "LAYER 2 COMPLETED" << std::endl;
    ct = polynomial_sigmoid_fhe(ct, cc, SIGMOID_COEFFS, 16);
    std::cout << "SIGMOID 2 COMPLETED" << std::endl;
    ct = linear_layer(ct, layers[2], cc);
    return ct;
}
