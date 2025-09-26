#pragma once

#include <cctype>
#include <cstdio>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "lazy/serialization/serializable.h"

namespace lazy {
namespace serialization {

/**
 * Core JSON implementation with DOM and parsing functionality.
 * Handles JSON parsing, tree manipulation, and serialization.
 */
class LazyJson {
 public:
  struct JsonValue {
    enum class Type : uint8_t { Null, String, Number, Bool, Object, Array };

    Type type = Type::Null;

    // Raw JSON value - used for both serialization and deserialization
    std::string rawValue;  // e.g., "123", "\"hello\"", "true", "null"

    // For objects and arrays
    std::unordered_map<std::string, std::unique_ptr<JsonValue>> objectChildren;
    std::vector<std::unique_ptr<JsonValue>> arrayChildren;

    JsonValue() = default;
    JsonValue(Type t) : type(t) {}

    // Move-only to avoid expensive copies
    JsonValue(JsonValue&& other) noexcept = default;
    JsonValue& operator=(JsonValue&& other) noexcept = default;
    JsonValue(const JsonValue&) = delete;
    JsonValue& operator=(const JsonValue&) = delete;
  };

  using NodeType = JsonValue*;

 private:
  // Single root - adapter is either serializing OR deserializing, never both
  std::unique_ptr<JsonValue> root_;

  // For deserialization: keep source JSON for on-demand parsing
  std::string sourceJson_;

 public:
  // Serialization constructor
  LazyJson() { root_ = std::make_unique<JsonValue>(JsonValue::Type::Object); }

  // Deserialization constructor
  explicit LazyJson(std::istream& stream) {
    // Read entire JSON into memory for lazy parsing
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    sourceJson_ = buffer.str();

    if (sourceJson_.empty()) {
      root_ = std::make_unique<JsonValue>(JsonValue::Type::Object);
    } else {
      root_ = parseJsonToTree(sourceJson_);
    }
  }

  // Get the JSON string representation of the tree
  std::string toJsonString() const { return valueToJsonString(*root_); }

  // Check if in serialization mode
  bool isSerializing() const { return sourceJson_.empty(); }

  NodeType root() { return root_.get(); }

  NodeType getChild(NodeType node, const std::string& key) {
    if (key.empty()) return node;  // Empty key means use same node
    if (!node || isSerializing()) return nullptr;

    if (node->type != JsonValue::Type::Object) return nullptr;

    auto it = node->objectChildren.find(key);
    return (it != node->objectChildren.end()) ? it->second.get() : nullptr;
  }

  NodeType addChild(NodeType node, const std::string& key) {
    if (key.empty()) return node;  // Empty key means use same node (for array elements)
    if (!node || !isSerializing()) return nullptr;

    if (node->type != JsonValue::Type::Object) {
      node->type = JsonValue::Type::Object;
    }

    // Create and return child node
    node->objectChildren[key] = std::make_unique<JsonValue>();
    return node->objectChildren[key].get();
  }

  bool isObject(NodeType node) { return node && node->type == JsonValue::Type::Object; }

  void setObject(NodeType node) { node->type = JsonValue::Type::Object; }

  bool isArray(NodeType node) { return node && node->type == JsonValue::Type::Array; }

  void setArray(NodeType node, size_t size) {
    if (!node) return;

    node->type = JsonValue::Type::Array;
    node->arrayChildren.reserve(size);  // Pre-allocate for known size
  }

  size_t getArraySize(NodeType node) {
    if (!node || isSerializing() || node->type != JsonValue::Type::Array) {
      return 0;
    }
    return node->arrayChildren.size();
  }

  NodeType getArrayElement(NodeType node, size_t index) {
    if (!node || isSerializing() || node->type != JsonValue::Type::Array) {
      return nullptr;
    }
    if (index >= node->arrayChildren.size()) {
      return nullptr;
    }
    return node->arrayChildren[index].get();
  }

  NodeType addArrayElement(NodeType node) {
    if (!node || !isSerializing()) return nullptr;

    if (node->type != JsonValue::Type::Array) {
      node->type = JsonValue::Type::Array;
    }

    // Create and return new array element
    node->arrayChildren.push_back(std::make_unique<JsonValue>());
    return node->arrayChildren.back().get();
  }

  // Lazy type conversion - parse on-demand from raw JSON
  template <typename T>
  T getValue(NodeType node) {
    if (!node) return T{};

    const std::string& jsonStr = node->rawValue;

    if constexpr (std::is_same_v<T, std::string>) {
      if (node->type == JsonValue::Type::String && jsonStr.length() >= 2 &&
          jsonStr.front() == '"' && jsonStr.back() == '"') {
        return unescapeJsonString(jsonStr.substr(1, jsonStr.length() - 2));
      }
      return T{};  // Default for type mismatch
    } else if constexpr (std::is_same_v<T, bool>) {
      if (node->type == JsonValue::Type::Bool) {
        return jsonStr == "true";
      }
      return false;  // Default for type mismatch
    } else if constexpr (std::is_arithmetic_v<T>) {
      if (node->type == JsonValue::Type::Number) {
        try {
          if constexpr (std::is_floating_point_v<T>) {
            return static_cast<T>(std::stod(jsonStr));
          } else {
            return static_cast<T>(std::stoll(jsonStr));
          }
        } catch (...) {
          return T{};  // Default for parse error
        }
      }
      return T{};  // Default for type mismatch
    }

    return T{};
  }

  template <typename T>
  void setValue(NodeType node, const T& value) {
    if (!node || !isSerializing()) return;

    if constexpr (std::is_same_v<T, std::string>) {
      node->type = JsonValue::Type::String;
      node->rawValue = "\"" + escapeJsonString(value) + "\"";  // Store as JSON string
    } else if constexpr (std::is_same_v<T, bool>) {
      node->type = JsonValue::Type::Bool;
      node->rawValue = value ? "true" : "false";
    } else if constexpr (std::is_arithmetic_v<T>) {
      node->type = JsonValue::Type::Number;
      node->rawValue = std::to_string(value);
    }
  }

 private:
  // Parse JSON string into tree structure
  std::unique_ptr<JsonValue> parseJsonToTree(const std::string& jsonStr) {
    JsonParser parser(jsonStr);
    return parser.parse();
  }

  // Convert JsonValue tree to JSON string (for serialization)
  std::string valueToJsonString(const JsonValue& value) const {
    switch (value.type) {
      case JsonValue::Type::Null:
        return "null";

      case JsonValue::Type::Bool:
      case JsonValue::Type::Number:
      case JsonValue::Type::String:
        return value.rawValue;  // Already in correct JSON format

      case JsonValue::Type::Array: {
        std::string result = "[";
        for (size_t i = 0; i < value.arrayChildren.size(); ++i) {
          if (i > 0) result += ",";
          result += valueToJsonString(*value.arrayChildren[i]);
        }
        result += "]";
        return result;
      }

      case JsonValue::Type::Object: {
        std::string result = "{";
        bool first = true;
        for (const auto& [key, child] : value.objectChildren) {
          if (!first) result += ",";
          first = false;
          result += "\"" + escapeJsonString(key) + "\":" + valueToJsonString(*child);
        }
        result += "}";
        return result;
      }
    }
    return "null";
  }

  // JSON string utility functions
  std::string escapeJsonString(const std::string& str) const {
    std::string result;
    result.reserve(str.size() + str.size() / 10);  // Reserve extra for escapes

    for (char ch : str) {
      switch (ch) {
        case '"':
          result += "\\\"";
          break;
        case '\\':
          result += "\\\\";
          break;
        case '\b':
          result += "\\b";
          break;
        case '\f':
          result += "\\f";
          break;
        case '\n':
          result += "\\n";
          break;
        case '\r':
          result += "\\r";
          break;
        case '\t':
          result += "\\t";
          break;
        default:
          if (ch < 32) {
            char buffer[8];
            snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned char>(ch));
            result += buffer;
          } else {
            result += ch;
          }
          break;
      }
    }

    return result;
  }

  std::string unescapeJsonString(std::string_view str) const {
    std::string result;
    result.reserve(str.size());

    for (size_t i = 0; i < str.size(); ++i) {
      if (str[i] == '\\' && i + 1 < str.size()) {
        switch (str[i + 1]) {
          case '"':
            result += '"';
            i++;
            break;
          case '\\':
            result += '\\';
            i++;
            break;
          case '/':
            result += '/';
            i++;
            break;
          case 'b':
            result += '\b';
            i++;
            break;
          case 'f':
            result += '\f';
            i++;
            break;
          case 'n':
            result += '\n';
            i++;
            break;
          case 'r':
            result += '\r';
            i++;
            break;
          case 't':
            result += '\t';
            i++;
            break;
          case 'u':
            if (i + 5 < str.size()) {
              result +=
                  static_cast<char>(std::stoul(std::string(str.substr(i + 2, 4)), nullptr, 16));
              i += 5;
            } else {
              result += str[i];
            }
            break;
          default:
            result += str[i];
            break;
        }
      } else {
        result += str[i];
      }
    }

    return result;
  }

  // JSON parser optimized for simplicity
  class JsonParser {
   private:
    const std::string& input_;
    size_t position_;

    void skipWhitespace() {
      while (position_ < input_.size() && std::isspace(input_[position_])) {
        ++position_;
      }
    }

    char currentChar() const { return (position_ < input_.size()) ? input_[position_] : '\0'; }

    char consumeChar() { return (position_ < input_.size()) ? input_[position_++] : '\0'; }

    bool consumeIf(char expected) {
      if (currentChar() == expected) {
        consumeChar();
        return true;
      }
      return false;
    }

   public:
    JsonParser(const std::string& input) : input_(input), position_(0) {}

    std::unique_ptr<JsonValue> parse() {
      skipWhitespace();
      return parseValue();
    }

   private:
    std::unique_ptr<JsonValue> parseValue() {
      skipWhitespace();
      uint32_t startPos = position_;

      switch (currentChar()) {
        case 'n':
          return parseNull(startPos);
        case 't':
        case 'f':
          return parseBool(startPos);
        case '"':
          return parseString(startPos);
        case '[':
          return parseArray();
        case '{':
          return parseObject();
        default:
          if (currentChar() == '-' || std::isdigit(currentChar())) {
            return parseNumber(startPos);
          }
          // Invalid JSON - return null
          return std::make_unique<JsonValue>(JsonValue::Type::Null);
      }
    }

    std::unique_ptr<JsonValue> parseNull(uint32_t startPos) {
      if (input_.substr(position_, 4) == "null") {
        position_ += 4;
      }
      auto node = std::make_unique<JsonValue>(JsonValue::Type::Null);
      node->rawValue = input_.substr(startPos, position_ - startPos);
      return node;
    }

    std::unique_ptr<JsonValue> parseBool(uint32_t startPos) {
      if (input_.substr(position_, 4) == "true") {
        position_ += 4;
      } else if (input_.substr(position_, 5) == "false") {
        position_ += 5;
      }
      auto node = std::make_unique<JsonValue>(JsonValue::Type::Bool);
      node->rawValue = input_.substr(startPos, position_ - startPos);
      return node;
    }

    std::unique_ptr<JsonValue> parseString(uint32_t startPos) {
      if (consumeIf('"')) {
        // Find end of string, handling escapes
        while (currentChar() != '"' && currentChar() != '\0') {
          if (currentChar() == '\\') {
            consumeChar();  // Skip escape character
            if (currentChar() != '\0') {
              consumeChar();  // Skip escaped character
            }
          } else {
            consumeChar();
          }
        }
        consumeIf('"');  // Consume closing quote
      }

      auto node = std::make_unique<JsonValue>(JsonValue::Type::String);
      node->rawValue = input_.substr(startPos, position_ - startPos);
      return node;
    }

    std::unique_ptr<JsonValue> parseNumber(uint32_t startPos) {
      if (currentChar() == '-') consumeChar();

      // Must have at least one digit
      if (!std::isdigit(currentChar())) {
        auto node = std::make_unique<JsonValue>(JsonValue::Type::Number);
        node->rawValue = input_.substr(startPos, position_ - startPos);
        return node;
      }

      // Integer part
      while (std::isdigit(currentChar())) consumeChar();

      // Fractional part
      if (currentChar() == '.') {
        consumeChar();
        while (std::isdigit(currentChar())) consumeChar();
      }

      // Exponent part
      if (currentChar() == 'e' || currentChar() == 'E') {
        consumeChar();
        if (currentChar() == '+' || currentChar() == '-') consumeChar();
        while (std::isdigit(currentChar())) consumeChar();
      }

      auto node = std::make_unique<JsonValue>(JsonValue::Type::Number);
      node->rawValue = input_.substr(startPos, position_ - startPos);
      return node;
    }

    std::unique_ptr<JsonValue> parseArray() {
      if (!consumeIf('[')) {
        return std::make_unique<JsonValue>(JsonValue::Type::Array);
      }

      auto arrayValue = std::make_unique<JsonValue>(JsonValue::Type::Array);
      skipWhitespace();

      if (consumeIf(']')) {
        return arrayValue;
      }

      // Parse array elements
      while (true) {
        arrayValue->arrayChildren.push_back(parseValue());
        skipWhitespace();

        if (consumeIf(']')) break;
        if (!consumeIf(',')) break;  // Invalid JSON, but handle gracefully
        skipWhitespace();
      }

      return arrayValue;
    }

    std::unique_ptr<JsonValue> parseObject() {
      if (!consumeIf('{')) {
        return std::make_unique<JsonValue>(JsonValue::Type::Object);
      }

      auto objectValue = std::make_unique<JsonValue>(JsonValue::Type::Object);
      skipWhitespace();

      if (consumeIf('}')) {
        return objectValue;
      }

      // Parse key-value pairs
      while (true) {
        skipWhitespace();

        // Parse key (must be string)
        auto keyValue = parseString(position_);
        if (keyValue->type != JsonValue::Type::String) break;

        // Extract key from raw value (remove quotes)
        std::string key = keyValue->rawValue;
        if (key.length() >= 2 && key.front() == '"' && key.back() == '"') {
          key = key.substr(1, key.length() - 2);  // Simple unquoting
        }

        skipWhitespace();
        if (!consumeIf(':')) break;

        // Parse value
        objectValue->objectChildren[key] = parseValue();
        skipWhitespace();

        if (consumeIf('}')) break;
        if (!consumeIf(',')) break;  // Invalid JSON, but handle gracefully
      }

      return objectValue;
    }
  };
};

/**
 * JSON serialization adapter that bridges the AdapterType interface to LazyJson.
 * Handles streaming and delegates all JSON operations to the LazyJson core.
 */
class LazyJsonAdapter {
 public:
  using NodeType = LazyJson::JsonValue*;

 private:
  std::unique_ptr<LazyJson> json_;
  std::ostream* outputStream_ = nullptr;

 public:
  // Serialization constructor
  LazyJsonAdapter() : json_(std::make_unique<LazyJson>()) {}

  // Deserialization constructor
  explicit LazyJsonAdapter(std::istream& stream) : json_(std::make_unique<LazyJson>(stream)) {}

  // Serialization with output stream
  explicit LazyJsonAdapter(std::ostream& stream) : LazyJsonAdapter() { outputStream_ = &stream; }

  void finishSerialization() const {
    if (outputStream_) {
      *outputStream_ << json_->toJsonString();
    }
  }

  void finishDeserialization() {
    // No cleanup needed
  }

  NodeType root() { return json_->root(); }

  NodeType getChild(NodeType node, const std::string& key) { return json_->getChild(node, key); }

  NodeType addChild(NodeType node, const std::string& key) { return json_->addChild(node, key); }

  bool isObject(NodeType node) { return json_->isObject(node); }

  void setObject(NodeType node) { json_->setObject(node); }

  bool isArray(NodeType node) { return json_->isArray(node); }

  void setArray(NodeType node, size_t size) { json_->setArray(node, size); }

  size_t getArraySize(NodeType node) { return json_->getArraySize(node); }

  NodeType getArrayElement(NodeType node, size_t index) {
    return json_->getArrayElement(node, index);
  }

  NodeType addArrayElement(NodeType node) { return json_->addArrayElement(node); }

  template <typename T>
  T getValue(NodeType node) {
    return json_->getValue<T>(node);
  }

  template <typename T>
  void setValue(NodeType node, const T& value) {
    json_->setValue(node, value);
  }
};

// Convenience aliases
template <typename T>
using LazyJsonSerializable = Serializable<T, LazyJsonAdapter>;

}  // namespace serialization

// Global alias for easier access
template <typename T>
using LazyJsonSerializable = lazy::serialization::LazyJsonSerializable<T>;

}  // namespace lazy