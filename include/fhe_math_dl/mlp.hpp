#pragma once

#include "fhe_math_dl/fhe_act.hpp"
#include "glaze/glaze.hpp"
#include "openfhe.h"
#include <map>
#include <string>
#include <vector>

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

  /// Load weights from a JSON file exported by PyTorch (flat dict of
  /// "layer.weight" / "layer.bias" arrays).
  void load_weights(const std::string &filename);

  /// Run a full forward pass on an encrypted input ciphertext.
  Ciphertext<DCRTPoly> forward(const Ciphertext<DCRTPoly> &input,
                               const CryptoContext<DCRTPoly> &cc);

private:
  std::vector<LinearLayer> layers;

  Ciphertext<DCRTPoly> linear_layer(const Ciphertext<DCRTPoly> &ct_input,
                                    const LinearLayer &layer,
                                    const CryptoContext<DCRTPoly> &cc,
                                    bool repeat_input = false);

  void add_layer(const std::vector<std::vector<double>> &W,
                 const std::vector<double> &b);
};
