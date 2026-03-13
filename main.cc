#include <vector>
#define PROFILE

#include "openfhe.h"
#include <cmath>
#include "mlp.hpp"
#include "fhe_act.hpp"
#include "glaze/glaze.hpp"
#include <iostream>
#include <iomanip>
#include <map>
#include <chrono>

using namespace lbcrypto;

double evaluate(
    MLP& mlp,
    const CryptoContext<DCRTPoly>& cc,
    const KeyPair<DCRTPoly>& keys,
    const std::vector<std::vector<double>>& X,
    const std::vector<double>& Y
) {
    int correct = 0;
    int skipped = 0;
    int total = (int)X.size();
    int num_classes = 3;

    // confusion matrix [actual][predicted]
    std::vector<std::vector<int>> conf(num_classes, std::vector<int>(num_classes, 0));

    for (int i = 0; i < total; i++) {
        try {
            Plaintext pt = cc->MakeCKKSPackedPlaintext(X[i]);
            auto ct = cc->Encrypt(keys.publicKey, pt);
            auto ct_out = mlp.forward(ct, cc);

            Plaintext result;
            cc->Decrypt(keys.secretKey, ct_out, &result);
            result->SetLength(3);
            std::vector<double> v = result->GetRealPackedValue();

            int pred  = (int)std::distance(v.begin(), std::max_element(v.begin(), v.end()));
            int label = (int)Y[i];

            conf[label][pred]++;
            if (pred == label) correct++;

            std::cout << "sample " << i
                      << " pred: " << pred
                      << " label: " << label
                      << (pred == label ? " ✓" : " ✗")
                      << std::endl;

        } catch (const std::exception& e) {
            std::cout << "sample " << i << " SKIPPED: " << e.what() << std::endl;
            skipped++;
        }
    }

    int evaluated = total - skipped;
    double accuracy = (double)correct / evaluated * 100.0;

    // print confusion matrix
    std::cout << "\n=== Confusion Matrix ===" << std::endl;
    std::cout << "          pred:0  pred:1  pred:2" << std::endl;
    for (int i = 0; i < num_classes; i++) {
        std::cout << "actual:" << i << "  ";
        for (int j = 0; j < num_classes; j++)
            std::cout << std::setw(6) << conf[i][j] << "  ";
        std::cout << std::endl;
    }

    // per class precision/recall
    std::cout << "\n=== Per Class ===" << std::endl;
    std::cout << std::setw(8) << "class"
              << std::setw(10) << "precision"
              << std::setw(10) << "recall" << std::endl;
    for (int c = 0; c < num_classes; c++) {
        int tp = conf[c][c];
        int col_sum = 0, row_sum = 0;
        for (int k = 0; k < num_classes; k++) {
            col_sum += conf[k][c];  // predicted as c
            row_sum += conf[c][k];  // actual c
        }
        double precision = col_sum > 0 ? (double)tp / col_sum * 100.0 : 0.0;
        double recall    = row_sum > 0 ? (double)tp / row_sum * 100.0 : 0.0;
        std::cout << std::setw(8) << c
                  << std::setw(9) << std::fixed << std::setprecision(1) << precision << "%"
                  << std::setw(9) << std::fixed << std::setprecision(1) << recall    << "%"
                  << std::endl;
    }

    std::cout << "\nTotal:     " << total     << std::endl;
    std::cout << "Skipped:   " << skipped    << std::endl;
    std::cout << "Evaluated: " << evaluated  << std::endl;
    std::cout << "Accuracy:  " << correct << "/" << evaluated
              << " = " << accuracy << "%" << std::endl;

    return accuracy;
}

const std::string filename = "/Users/bellian/CEPP/mlp/iris_weights.json";
const std::string X_test_filename = "/Users/bellian/CEPP/mlp/X_test.json";
const std::string Y_test_filename = "/Users/bellian/CEPP/mlp/Y_test.json";

struct X_test {
    std::vector<std::vector<double>> data;
};

struct Y_test {
    std::vector<double> data;
};

std::vector<int> gen_rot_keys(int batchSize) {
    std::vector<int> keys;
    for (int i = -(batchSize/2); i <= batchSize/2; ++i) {
        if (i != 0) keys.push_back(i);
        keys.push_back(i);
    }
    return keys;
}

template<typename T>
void load_data(T& data, const std::string& filename) {
    std::string buffer;
    auto err = glz::read_file_json(data, filename, buffer);
    if (err) {
        std::cout << glz::format_error(err, buffer) << '\n';
        return;
    }
}

int main() {
    X_test x;
    load_data(x, X_test_filename);

    Y_test y_test;
    load_data(y_test, Y_test_filename);

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
    const std::vector<int> indexLists = gen_rot_keys(batchSize);
    cc->EvalRotateKeyGen(keys.secretKey, indexLists);
    auto start = std::chrono::high_resolution_clock::now();
    evaluate(mlp, cc, keys, x.data, y_test.data);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time: " << elapsed.count() << " seconds\n";

    return 0;
}
