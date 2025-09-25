#pragma once

#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

/**
 * Mock serialization context for testing Serializable functionality
 * Records all operations for verification in tests
 */
class MockContext {
 public:
  struct MockNode {
    std::string key;
    std::map<std::string, MockNode*> children;
    std::vector<MockNode*> arrayElements;
    std::variant<int, double, std::string, bool> value;
    bool isObjectNode = false;
    bool isArrayNode = false;
    bool hasValue = false;

    ~MockNode() {
      for (auto& [k, child] : children) {
        delete child;
      }
      for (auto* element : arrayElements) {
        delete element;
      }
    }
  };

  using NodeType = MockNode*;

  MockContext() : root_(new MockNode()) {}
  MockContext(std::istream& /*stream*/) : root_(new MockNode()) {
    // Mock deserialization from stream - populate with test data if needed
  }
  ~MockContext() { delete root_; }

  void toStream(std::ostream& stream) const {
    // Mock output to stream for testing
    stream << "mock_output";
  }

  NodeType root() { return root_; }

  NodeType getChild(NodeType node, const std::string& key) {
    if (key.empty()) return node;

    auto it = node->children.find(key);
    return (it != node->children.end()) ? it->second : nullptr;
  }

  NodeType addChild(NodeType node, const std::string& key) {
    if (key.empty()) return node;

    auto* child = new MockNode();
    child->key = key;
    node->children[key] = child;
    return child;
  }

  bool isObject(NodeType node) { return node->isObjectNode; }
  void setObject(NodeType node) {
    node->isObjectNode = true;
    operations_.emplace_back("setObject", node->key);
  }

  bool isArray(NodeType node) { return node->isArrayNode; }
  void setArray(NodeType node, size_t size) {
    node->isArrayNode = true;
    node->arrayElements.reserve(size);
    operations_.emplace_back("setArray", node->key + "[" + std::to_string(size) + "]");
  }

  size_t getArraySize(NodeType node) { return node->arrayElements.size(); }

  NodeType getArrayElement(NodeType node, size_t index) {
    return (index < node->arrayElements.size()) ? node->arrayElements[index] : nullptr;
  }

  NodeType addArrayElement(NodeType node) {
    auto* element = new MockNode();
    node->arrayElements.push_back(element);
    return element;
  }

  template <typename T>
  T getValue(NodeType node) {
    if (node && node->hasValue) {
      return std::get<T>(node->value);
    }
    return T{};
  }

  template <typename T>
  void setValue(NodeType node, T value) {
    node->value = value;
    node->hasValue = true;
    if constexpr (std::is_same_v<T, std::string>) {
      operations_.emplace_back("setValue", node->key + "=" + value);
    } else if constexpr (std::is_arithmetic_v<T>) {
      operations_.emplace_back("setValue", node->key + "=" + std::to_string(value));
    } else {
      operations_.emplace_back("setValue", node->key + "=(value)");
    }
  }

  // Test utilities
  const std::vector<std::pair<std::string, std::string>>& getOperations() const {
    return operations_;
  }

  void clearOperations() { operations_.clear(); }

  bool hasOperation(const std::string& op, const std::string& detail = "") {
    for (const auto& [operation, operationDetail] : operations_) {
      if (operation == op &&
          (detail.empty() || operationDetail.find(detail) != std::string::npos)) {
        return true;
      }
    }
    return false;
  }

 private:
  MockNode* root_;
  std::vector<std::pair<std::string, std::string>> operations_;
};
