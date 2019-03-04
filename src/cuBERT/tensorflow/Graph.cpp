#include "Graph.h"

#include <stdexcept>
#include <fstream>
#include <iostream>

namespace cuBERT {
    Graph::Graph(const char *filename) {
        std::ifstream input(filename);
        if (!input) {
            // try to check file exist
            // https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-c
            throw std::invalid_argument("model file not found");
        }

        graphDef.ParseFromIstream(&input);
        input.close();
        std::cerr << "model loaded from: " << filename << std::endl;

        for (const auto &nodeDef : graphDef.node()) {
            if (!nodeDef.attr().count("value")) {
                continue;
            }

            const tensorflow::AttrValue &attrValue = nodeDef.attr().at("value");
            if (attrValue.value_case() != tensorflow::AttrValue::ValueCase::kTensor) {
                continue;
            }

            const tensorflow::TensorProto &tensorProto = attrValue.tensor();
            if (tensorProto.dtype() != tensorflow::DataType::DT_FLOAT) {
                continue;
            }
            if (tensorProto.tensor_content().empty()) {
                continue;
            }
            var[nodeDef.name()] = (float *) tensorProto.tensor_content().data();

            if (nodeDef.name() == "bert/embeddings/word_embeddings") {
                vocab_size = tensorProto.tensor_shape().dim(0).size();
                hidden_size = tensorProto.tensor_shape().dim(1).size();
            } else if (nodeDef.name() == "bert/embeddings/token_type_embeddings") {
                type_vocab_size = tensorProto.tensor_shape().dim(0).size();
                hidden_size = tensorProto.tensor_shape().dim(1).size();
            } else if (nodeDef.name() == "bert/encoder/layer_0/intermediate/dense/bias") {
                intermediate_size = tensorProto.tensor_shape().dim(0).size();
            }
        }

        if (var.empty()) {
            throw std::invalid_argument("model file invalid");
        }
    }
}