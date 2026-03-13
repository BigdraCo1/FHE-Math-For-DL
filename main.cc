#define PROFILE

#include "openfhe.h"
#include <cmath>
#include "mlp.hpp"
#include "fhe_act.hpp"
#include "glaze/glaze.hpp"
#include <iostream>
#include <iomanip>
#include <map>

using namespace lbcrypto;

const std::string filename = "/Users/bellian/CEPP/mlp/iris_weights.json";

std::vector<int> gen_rot_keys(int batchSize) {
    std::vector<int> keys;
    for (int i = -(batchSize/2); i <= batchSize/2; ++i) {
        if (i != 0) keys.push_back(i);
        keys.push_back(i);
    }
    return keys;
}

int main() {
    MLP mlp;
    mlp.load_weights(filename);

    uint32_t multDepth   = 16;
    uint32_t scaleModSize = 50;
    uint32_t batchSize   = 16;

    CCParams<CryptoContextCKKSRNS> parameters;
    parameters.SetMultiplicativeDepth(multDepth);
    parameters.SetScalingModSize(scaleModSize);
    parameters.SetBatchSize(batchSize);

    CryptoContext<DCRTPoly> cc = GenCryptoContext(parameters);
    cc->Enable(PKE);
    cc->Enable(KEYSWITCH);
    cc->Enable(LEVELEDSHE);

    auto keys = cc->KeyGen();
    cc->EvalMultKeyGen(keys.secretKey);
    // Rot Keys Gen
    // EVEN --> -q/2 < indexLists <= q/2
    // ODD  --> -(q-1)/2 <= indexLists <= (q-1)/2
    const std::vector<int> indexLists = gen_rot_keys(batchSize);
    std::cout << indexLists << std::endl;
    cc->EvalRotateKeyGen(keys.secretKey, indexLists);

    auto print_ct = [&](const std::string& label, const Ciphertext<DCRTPoly>& ct, int len) {
        Plaintext pt;
        cc->Decrypt(keys.secretKey, ct, &pt);
        pt->SetLength(len);
        std::cout << label << ": " << pt << std::endl;
    };

    std::vector<double> x1 = {-0.1331,  1.6508, -1.1614, -1.1791};
    // Encode
    Plaintext ptxt1 = cc->MakeCKKSPackedPlaintext(x1);
    std::cout << "Input x1: " << ptxt1 << std::endl;

    // Encrypt
    auto c1 = cc->Encrypt(keys.publicKey, ptxt1);

    // Forward
    // auto cPred = mlp.forward(c1, cc);
    auto ct = mlp.fwd_linear1(c1, cc);  print_ct("Linear 1", ct, 16);
    ct = mlp.fwd_sigmoid1(ct, cc);      print_ct("Sigmoid 1", ct, 16);
    ct = mlp.fwd_linear2(ct, cc);       print_ct("Linear 2", ct, 16);
    ct = mlp.fwd_sigmoid2(ct, cc);      print_ct("Sigmoid 2", ct, 16);
    ct = mlp.fwd_linear3(ct, cc);       print_ct("Linear 3", ct, 3);

    // Decryption and output
    Plaintext result;
    // We set the cout precision to 8 decimal digits for a nicer output.
    // If you want to see the error/noise introduced by CKKS, bump it up
    // to 15 and it should become visible.
    std::cout.precision(8);

    cc->Decrypt(keys.secretKey, ct, &result);
    result->SetLength(3);
    std::cout << result << std::endl;

    return 0;
}
