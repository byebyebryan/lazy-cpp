#pragma once

#include <deque>
#include <fkYAML/node.hpp>
#include <istream>
#include <ostream>
#include <string>
#include <type_traits>

#include "lazy/serialization/serializable.h"

namespace lazy {
namespace serialization {

/**
 * YAML serialization adapter implementation using fkYAML.
 * Provides the AdapterType interface required by Serializable.
 */
class YamlAdapter {
 private:
  fkyaml::node document_;
  std::ostream* writeStream_ = nullptr;  // For serialization

 public:
  using NodeType = fkyaml::node*;

  YamlAdapter() { document_ = fkyaml::node::mapping(); }

  explicit YamlAdapter(std::istream& stream) { document_ = fkyaml::node::deserialize(stream); }

  // For serialization - store stream reference
  explicit YamlAdapter(std::ostream& stream) : writeStream_(&stream) {
    document_ = fkyaml::node::mapping();
  }

  // Clean finish methods for symmetric API
  void finishSerialization() const {
    if (writeStream_) {
      std::string yaml_string = fkyaml::node::serialize(document_);
      *writeStream_ << yaml_string;
    }
  }

  void finishDeserialization() {
    // No-op for YAML - no cleanup needed after deserialization
  }

  NodeType root() { return &document_; }

  NodeType getChild(NodeType node, const std::string& key) {
    // empty key means the node itself
    if (key.empty()) {
      return node;
    }
    if (node && node->is_mapping() && node->contains(key)) {
      return &((*node)[key]);
    }
    return nullptr;
  }

  NodeType addChild(NodeType node, const std::string& key) {
    // empty key means the node itself
    if (key.empty()) {
      return node;
    }
    (*node)[key] = fkyaml::node();
    return &((*node)[key]);
  }

  bool isObject(NodeType node) { return node && node->is_mapping(); }
  void setObject(NodeType node) { *node = fkyaml::node::mapping(); }

  bool isArray(NodeType node) { return node && node->is_sequence(); }
  void setArray(NodeType node, size_t size) {
    *node = fkyaml::node::sequence();
    // YAML sequences grow dynamically, no need to pre-reserve
  }

  size_t getArraySize(NodeType node) { return node ? node->size() : 0; }

  NodeType getArrayElement(NodeType node, size_t index) {
    if (node && node->is_sequence() && index < node->size()) {
      return &((*node)[index]);
    }
    return nullptr;
  }

  NodeType addArrayElement(NodeType node) {
    auto& seq = node->get_value_ref<fkyaml::node::sequence_type&>();
    seq.emplace_back();
    return &seq.back();
  }

  template <typename T>
  T getValue(NodeType node) {
    if (!node) return T{};

    try {
      return node->get_value<T>();
    } catch (const fkyaml::exception&) {
      return T{};  // Return default value if conversion fails
    }
  }

  template <typename T>
  void setValue(NodeType node, const T& value) {
    if (node) {
      *node = value;
    }
  }
};

// Convenience alias for YAML serializable types
template <typename T>
using YamlSerializable = Serializable<T, YamlAdapter>;

}  // namespace serialization

// Global alias for easier access
template <typename T>
using YamlSerializable = serialization::YamlSerializable<T>;

}  // namespace lazy
