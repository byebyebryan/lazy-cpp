#pragma once

#include <yaml-cpp/yaml.h>

#include <deque>
#include <istream>
#include <ostream>
#include <string>
#include <type_traits>

#include "lazy/serialization/serializable.h"

namespace lazy {
namespace serialization {

/**
 * YAML serialization adapter implementation using yaml-cpp.
 * Provides the AdapterType interface required by Serializable.
 */
class YamlAdapter {
 private:
  YAML::Node document_;
  std::ostream* writeStream_ = nullptr;  // For serialization
  // Simple node storage to avoid pointer invalidation issues
  // Using deque instead of vector to avoid pointer invalidation on growth
  mutable std::deque<YAML::Node> nodeStorage_;

 public:
  using NodeType = YAML::Node*;

  YamlAdapter() { document_ = YAML::Node(YAML::NodeType::Map); }

  explicit YamlAdapter(std::istream& stream) { document_ = YAML::Load(stream); }

  // For serialization - store stream reference
  explicit YamlAdapter(std::ostream& stream) : writeStream_(&stream) {
    document_ = YAML::Node(YAML::NodeType::Map);
  }

  // Clean finish methods for symmetric API
  void finishSerialization() const {
    if (writeStream_) {
      *writeStream_ << document_;
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
    if (node && (*node)[key]) {
      // Store child node in our storage and return pointer to it
      nodeStorage_.push_back((*node)[key]);
      return &nodeStorage_.back();
    }
    return nullptr;
  }

  NodeType addChild(NodeType node, const std::string& key) {
    // empty key means the node itself
    if (key.empty()) {
      return node;
    }
    (*node)[key] = YAML::Node();
    // Store child node in our storage and return pointer to it
    nodeStorage_.push_back((*node)[key]);
    return &nodeStorage_.back();
  }

  bool isObject(NodeType node) { return node && node->IsMap(); }
  void setObject(NodeType node) { *node = YAML::Node(YAML::NodeType::Map); }

  bool isArray(NodeType node) { return node && node->IsSequence(); }
  void setArray(NodeType node, size_t size) {
    *node = YAML::Node(YAML::NodeType::Sequence);
    // YAML sequences grow dynamically, no need to pre-reserve
  }

  size_t getArraySize(NodeType node) { return node ? node->size() : 0; }

  NodeType getArrayElement(NodeType node, size_t index) {
    if (node && index < node->size()) {
      // Store array element in our storage and return pointer to it
      nodeStorage_.push_back((*node)[index]);
      return &nodeStorage_.back();
    }
    return nullptr;
  }

  NodeType addArrayElement(NodeType node) {
    node->push_back(YAML::Node());
    // Store array element in our storage and return pointer to it
    nodeStorage_.push_back((*node)[node->size() - 1]);
    return &nodeStorage_.back();
  }

  template <typename T>
  T getValue(NodeType node) {
    if (!node) return T{};

    try {
      return node->as<T>();
    } catch (const YAML::Exception&) {
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
