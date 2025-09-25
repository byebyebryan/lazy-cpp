#pragma once

#include <istream>
#include <map>
#include <ostream>
#include <string>
#include <type_traits>

#include "lazy/serialization/serializable.h"

namespace lazy {
namespace serialization {

/**
 * Text serialization adapter implementation using key-value pairs.
 * Provides the AdapterType interface required by Serializable.
 *
 * Format:
 * - Simple values: key = value
 * - Nested objects: parent.child.field = value
 * - Arrays: array.count = N, array.0 = value1, array.1 = value2, ...
 * - Strings are quoted, numbers/booleans are unquoted
 */
class TextAdapter {
 private:
  // We need to store actual path strings and return pointers to them
  mutable std::map<std::string, std::string> nodeCache_;
  // Track current index for each array during serialization
  mutable std::map<std::string, size_t> arrayIndices_;

  // For direct writing during serialization
  std::ostream* writeStream_ = nullptr;

 public:
  using NodeType = const std::string*;

  TextAdapter() = default;

  explicit TextAdapter(std::istream& stream) {
    std::string line;
    while (std::getline(stream, line)) {
      parseLine(line);
    }
  }

  // For serialization - write text directly to stream
  explicit TextAdapter(std::ostream& stream) : writeStream_(&stream) {}

  // Clean finish methods for symmetric API
  void finishSerialization() {
    // No-op for text - already writing directly to stream during setValue()
  }

  void finishDeserialization() {
    // No-op for text - no cleanup needed after deserialization
  }

  NodeType root() {
    nodeCache_.clear();
    arrayIndices_.clear();
    nodeCache_[""] = "";
    return &nodeCache_[""];
  }

  NodeType getChild(NodeType node, const std::string& key) {
    if (key.empty()) {
      return node;
    }
    std::string childPath = node->empty() ? key : *node + "." + key;
    // Check if this path actually exists in our data
    if (hasDataForPath(childPath)) {
      nodeCache_[childPath] = childPath;
      return &nodeCache_[childPath];
    }
    return nullptr;
  }

  NodeType addChild(NodeType node, const std::string& key) {
    if (key.empty()) {
      return node;
    }
    std::string childPath = node->empty() ? key : *node + "." + key;
    nodeCache_[childPath] = childPath;
    return &nodeCache_[childPath];
  }

  bool isObject(NodeType node) {
    if (!node) return false;
    // Check if any keys start with this node path
    std::string prefix = node->empty() ? "" : *node + ".";
    for (const auto& [key, value] : data_) {
      if (key.substr(0, prefix.length()) == prefix) {
        return true;
      }
    }
    return false;
  }

  void setObject(NodeType node) {
    // Objects are implicit in our format - no action needed
  }

  bool isArray(NodeType node) {
    if (!node) return false;
    std::string countKey = *node + ".count";
    return data_.find(countKey) != data_.end();
  }

  void setArray(NodeType node, size_t size) {
    if (!node) return;
    std::string countKey = *node + ".count";
    std::string countValue = std::to_string(size);

    if (writeStream_) {
      // Write directly to stream during serialization
      *writeStream_ << countKey << " = " << countValue << '\n';
    } else {
      // Store in map for deserialization
      data_[countKey] = countValue;
    }

    // Reset the array index counter for this array
    arrayIndices_[*node] = 0;
  }

  size_t getArraySize(NodeType node) {
    if (!node) return 0;
    std::string countKey = *node + ".count";
    auto it = data_.find(countKey);
    if (it != data_.end()) {
      return std::stoull(it->second);
    }
    return 0;
  }

  NodeType getArrayElement(NodeType node, size_t index) {
    if (!node) return nullptr;
    std::string elementPath = *node + "." + std::to_string(index);
    nodeCache_[elementPath] = elementPath;
    return &nodeCache_[elementPath];
  }

  NodeType addArrayElement(NodeType node) {
    if (!node) return nullptr;

    // Get or initialize the current index for this array
    const std::string& arrayPath = *node;
    size_t& currentIndex = arrayIndices_[arrayPath];

    // Create the indexed element path
    std::string elementPath = arrayPath + "." + std::to_string(currentIndex);
    nodeCache_[elementPath] = elementPath;

    // Increment the index for next call
    currentIndex++;

    return &nodeCache_[elementPath];
  }

  template <typename T>
  T getValue(NodeType node) {
    if (!node) return T{};
    auto it = data_.find(*node);
    if (it == data_.end()) {
      return T{};
    }

    const std::string& value = it->second;

    if constexpr (std::is_same_v<T, std::string>) {
      // Remove quotes from string values
      if (value.length() >= 2 && value.front() == '"' && value.back() == '"') {
        return value.substr(1, value.length() - 2);
      }
      return value;
    } else if constexpr (std::is_same_v<T, bool>) {
      return value == "true";
    } else if constexpr (std::is_integral_v<T>) {
      if constexpr (std::is_signed_v<T>) {
        return static_cast<T>(std::stoll(value));
      } else {
        return static_cast<T>(std::stoull(value));
      }
    } else if constexpr (std::is_floating_point_v<T>) {
      return static_cast<T>(std::stod(value));
    } else {
      static_assert(!std::is_same_v<T, T>, "Unsupported type for getValue");
    }
  }

  template <typename T>
  void setValue(NodeType node, const T& value) {
    if (!node) return;

    std::string textValue;
    if constexpr (std::is_same_v<T, std::string>) {
      // Quote string values
      textValue = "\"" + value + "\"";
    } else if constexpr (std::is_same_v<T, bool>) {
      textValue = value ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
      textValue = std::to_string(value);
    } else {
      static_assert(!std::is_same_v<T, T>, "Unsupported type for setValue");
    }

    if (writeStream_) {
      // Write directly to stream during serialization
      *writeStream_ << *node << " = " << textValue << '\n';
    } else {
      // Store in map for deserialization
      data_[*node] = textValue;
    }
  }

 private:
  std::map<std::string, std::string> data_;

  bool hasDataForPath(const std::string& path) const {
    // Check if we have any data that starts with this path
    if (data_.find(path) != data_.end()) {
      return true;
    }

    std::string prefix = path + ".";
    for (const auto& [key, value] : data_) {
      if (key.substr(0, prefix.length()) == prefix) {
        return true;
      }
    }
    return false;
  }

  void parseLine(const std::string& line) {
    // Skip empty lines and comments
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#') {
      return;
    }

    // Find the '=' separator
    size_t equalPos = trimmed.find('=');
    if (equalPos == std::string::npos) {
      return;
    }

    std::string key = trim(trimmed.substr(0, equalPos));
    std::string value = trim(trimmed.substr(equalPos + 1));

    if (!key.empty()) {
      data_[key] = value;
    }
  }

  std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(' ');
    if (first == std::string::npos) {
      return "";
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, last - first + 1);
  }
};

// Convenience alias for text serializable types
template <typename T>
using TextSerializable = Serializable<T, TextAdapter>;

}  // namespace serialization

// Global alias for easier access
template <typename T>
using TextSerializable = serialization::TextSerializable<T>;

}  // namespace lazy
