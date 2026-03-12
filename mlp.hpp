#include "openfhe.h"
#include "glaze/glaze.hpp"
#include <vector>
#include <map>
#include <string>
#include "fhe_act.hpp"

using namespace lbcrypto;

// single linear layer descriptor
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
    Ciphertext<DCRTPoly> fwd_linear1(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc);
    Ciphertext<DCRTPoly> fwd_sigmoid1(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc);
    Ciphertext<DCRTPoly> fwd_linear2(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc);
    Ciphertext<DCRTPoly> fwd_sigmoid2(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc);
    Ciphertext<DCRTPoly> fwd_linear3(const Ciphertext<DCRTPoly>& input, const CryptoContext<DCRTPoly>& cc);

private:
    std::vector<LinearLayer> layers;  // Linear(4,8), Linear(8,16), Linear(16,3)

    Ciphertext<DCRTPoly> linear_layer(
        const Ciphertext<DCRTPoly>& ct_input,
        const LinearLayer& layer,
        const CryptoContext<DCRTPoly>& cc
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
