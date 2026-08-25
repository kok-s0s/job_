#include <cstdint>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

struct Slice {
    const std::vector<std::uint8_t>& data;
    std::size_t begin;
    std::size_t end;
};

struct ValueInfo {
    std::string name;
    int elem_type = 0;
    std::vector<std::int64_t> shape;
};

struct TensorInfo {
    std::string name;
    int data_type = 0;
    std::vector<std::int64_t> dims;
    std::size_t raw_bytes = 0;
};

struct NodeInfo {
    std::string name;
    std::string op_type;
    std::vector<std::string> inputs;
    std::vector<std::string> outputs;
};

struct ModelInfo {
    std::string graph_name;
    std::int64_t ir_version = 0;
    std::int64_t opset_version = 0;
    std::vector<ValueInfo> inputs;
    std::vector<ValueInfo> outputs;
    std::vector<TensorInfo> initializers;
    std::vector<NodeInfo> nodes;
};

std::uint64_t readVarint(const Slice& slice, std::size_t& pos) {
    std::uint64_t value = 0;
    int shift = 0;
    while (pos < slice.end && shift <= 63) {
        const auto byte = slice.data[pos++];
        value |= static_cast<std::uint64_t>(byte & 0x7F) << shift;
        if ((byte & 0x80) == 0) {
            return value;
        }
        shift += 7;
    }
    throw std::runtime_error("malformed protobuf varint");
}

Slice readBytes(const Slice& slice, std::size_t& pos) {
    const auto length = static_cast<std::size_t>(readVarint(slice, pos));
    if (pos + length > slice.end) {
        throw std::runtime_error("protobuf length field exceeds file size");
    }
    const Slice nested{slice.data, pos, pos + length};
    pos += length;
    return nested;
}

std::string asString(const Slice& slice) {
    return std::string(slice.data.begin() + static_cast<std::ptrdiff_t>(slice.begin),
                       slice.data.begin() + static_cast<std::ptrdiff_t>(slice.end));
}

void skipField(const Slice& slice, std::size_t& pos, std::uint64_t wire_type) {
    switch (wire_type) {
        case 0:
            (void)readVarint(slice, pos);
            return;
        case 1:
            pos += 8;
            break;
        case 2:
            (void)readBytes(slice, pos);
            return;
        case 5:
            pos += 4;
            break;
        default:
            throw std::runtime_error("unsupported protobuf wire type");
    }
    if (pos > slice.end) {
        throw std::runtime_error("protobuf field exceeds file size");
    }
}

std::string joinShape(const std::vector<std::int64_t>& dims) {
    std::ostringstream out;
    out << "[";
    for (std::size_t i = 0; i < dims.size(); ++i) {
        if (i != 0) {
            out << ",";
        }
        out << dims[i];
    }
    out << "]";
    return out.str();
}

std::string elemTypeName(int elem_type) {
    return elem_type == 1 ? "float" : "elem_type_" + std::to_string(elem_type);
}

std::int64_t parseDimension(const Slice& slice) {
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 0) {
            return static_cast<std::int64_t>(readVarint(slice, pos));
        }
        skipField(slice, pos, wire);
    }
    return -1;
}

std::vector<std::int64_t> parseShape(const Slice& slice) {
    std::vector<std::int64_t> dims;
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 2) {
            dims.push_back(parseDimension(readBytes(slice, pos)));
        } else {
            skipField(slice, pos, wire);
        }
    }
    return dims;
}

void parseTensorType(const Slice& slice, ValueInfo& value) {
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 0) {
            value.elem_type = static_cast<int>(readVarint(slice, pos));
        } else if (field == 2 && wire == 2) {
            value.shape = parseShape(readBytes(slice, pos));
        } else {
            skipField(slice, pos, wire);
        }
    }
}

void parseType(const Slice& slice, ValueInfo& value) {
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 2) {
            parseTensorType(readBytes(slice, pos), value);
        } else {
            skipField(slice, pos, wire);
        }
    }
}

ValueInfo parseValueInfo(const Slice& slice) {
    ValueInfo value;
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 2) {
            value.name = asString(readBytes(slice, pos));
        } else if (field == 2 && wire == 2) {
            parseType(readBytes(slice, pos), value);
        } else {
            skipField(slice, pos, wire);
        }
    }
    return value;
}

TensorInfo parseTensor(const Slice& slice) {
    TensorInfo tensor;
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 0) {
            tensor.dims.push_back(static_cast<std::int64_t>(readVarint(slice, pos)));
        } else if (field == 2 && wire == 0) {
            tensor.data_type = static_cast<int>(readVarint(slice, pos));
        } else if (field == 8 && wire == 2) {
            tensor.name = asString(readBytes(slice, pos));
        } else if (field == 9 && wire == 2) {
            const auto raw = readBytes(slice, pos);
            tensor.raw_bytes = raw.end - raw.begin;
        } else {
            skipField(slice, pos, wire);
        }
    }
    return tensor;
}

NodeInfo parseNode(const Slice& slice) {
    NodeInfo node;
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 2) {
            node.inputs.push_back(asString(readBytes(slice, pos)));
        } else if (field == 2 && wire == 2) {
            node.outputs.push_back(asString(readBytes(slice, pos)));
        } else if (field == 3 && wire == 2) {
            node.name = asString(readBytes(slice, pos));
        } else if (field == 4 && wire == 2) {
            node.op_type = asString(readBytes(slice, pos));
        } else {
            skipField(slice, pos, wire);
        }
    }
    return node;
}

std::int64_t parseOpset(const Slice& slice) {
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 2 && wire == 0) {
            return static_cast<std::int64_t>(readVarint(slice, pos));
        }
        skipField(slice, pos, wire);
    }
    return 0;
}

void parseGraph(const Slice& slice, ModelInfo& model) {
    std::size_t pos = slice.begin;
    while (pos < slice.end) {
        const auto tag = readVarint(slice, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 2) {
            model.nodes.push_back(parseNode(readBytes(slice, pos)));
        } else if (field == 2 && wire == 2) {
            model.graph_name = asString(readBytes(slice, pos));
        } else if (field == 5 && wire == 2) {
            model.initializers.push_back(parseTensor(readBytes(slice, pos)));
        } else if (field == 11 && wire == 2) {
            model.inputs.push_back(parseValueInfo(readBytes(slice, pos)));
        } else if (field == 12 && wire == 2) {
            model.outputs.push_back(parseValueInfo(readBytes(slice, pos)));
        } else {
            skipField(slice, pos, wire);
        }
    }
}

ModelInfo parseModel(const std::vector<std::uint8_t>& data) {
    ModelInfo model;
    const Slice root{data, 0, data.size()};
    std::size_t pos = root.begin;
    while (pos < root.end) {
        const auto tag = readVarint(root, pos);
        const auto field = tag >> 3;
        const auto wire = tag & 0x07;
        if (field == 1 && wire == 0) {
            model.ir_version = static_cast<std::int64_t>(readVarint(root, pos));
        } else if (field == 7 && wire == 2) {
            parseGraph(readBytes(root, pos), model);
        } else if (field == 8 && wire == 2) {
            model.opset_version = parseOpset(readBytes(root, pos));
        } else {
            skipField(root, pos, wire);
        }
    }
    return model;
}

std::vector<std::uint8_t> readFile(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open ONNX model: " + path);
    }
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(input), {});
}

void requireModel(const ModelInfo& model) {
    if (model.graph_name.empty() || model.inputs.empty() || model.outputs.empty() || model.nodes.empty()) {
        throw std::runtime_error("model metadata is incomplete");
    }
}

void printModel(const std::string& path, const std::vector<std::uint8_t>& data, const ModelInfo& model) {
    std::cout << "[model] path=" << path << " bytes=" << data.size() << " ir_version=" << model.ir_version
              << " opset=" << model.opset_version << "\n";
    std::cout << "[model] graph=" << model.graph_name << " inputs=" << model.inputs.size()
              << " outputs=" << model.outputs.size() << " nodes=" << model.nodes.size()
              << " initializers=" << model.initializers.size() << "\n";
    for (const auto& input : model.inputs) {
        std::cout << "[input] name=" << input.name << " elem_type=" << elemTypeName(input.elem_type)
                  << " shape=" << joinShape(input.shape) << "\n";
    }
    for (const auto& output : model.outputs) {
        std::cout << "[output] name=" << output.name << " elem_type=" << elemTypeName(output.elem_type)
                  << " shape=" << joinShape(output.shape) << "\n";
    }
    for (const auto& tensor : model.initializers) {
        std::cout << "[initializer] name=" << tensor.name << " data_type=" << elemTypeName(tensor.data_type)
                  << " dims=" << joinShape(tensor.dims) << " raw_bytes=" << tensor.raw_bytes << "\n";
    }
    for (const auto& node : model.nodes) {
        std::cout << "[node] name=" << node.name << " op_type=" << node.op_type << "\n";
    }
}

}  // namespace

int main(int argc, char** argv) {
    try {
        const std::string path = argc > 1 ? argv[1] : "models/tiny_robot_score.onnx";
        const auto data = readFile(path);
        const auto model = parseModel(data);
        requireModel(model);
        printModel(path, data, model);
        std::cout << "[ok] onnx model metadata loaded\n";
    } catch (const std::exception& ex) {
        std::cerr << "[error] " << ex.what() << "\n";
        return 1;
    }
    return 0;
}
