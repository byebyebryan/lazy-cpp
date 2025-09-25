#pragma once

#include <rapidjson/document.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/writer.h>

#include <istream>
#include <ostream>
#include <string>
#include <type_traits>

#include "lazy/serialization/serializable.h"

namespace lazy {
namespace serialization {

/**
 * JSON serialization context implementation using RapidJSON.
 * Provides the ContextType interface required by Serializable.
 */
class RapidJsonContext {
 private:
  rapidjson::Document document_;
  std::ostream* writeStream_ = nullptr;  // For serialization

 public:
  using NodeType = rapidjson::Value*;

  RapidJsonContext() { document_.SetObject(); }

  explicit RapidJsonContext(std::istream& stream) {
    rapidjson::IStreamWrapper isw(stream);
    document_.ParseStream(isw);
  }

  // For serialization - store stream reference
  explicit RapidJsonContext(std::ostream& stream) : writeStream_(&stream) { document_.SetObject(); }

  // Clean finish methods for symmetric API
  void finishSerialization() const {
    if (writeStream_) {
      rapidjson::OStreamWrapper osw(*writeStream_);
      rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
      document_.Accept(writer);
    }
  }

  void finishDeserialization() {
    // No-op for JSON - no cleanup needed after deserialization
  }

  rapidjson::Document::AllocatorType& getAllocator() { return document_.GetAllocator(); }

  NodeType root() { return &document_; }
  NodeType getChild(NodeType node, const std::string& key) {
    // empty key means the node itself
    if (key.empty()) {
      return node;
    }
    if (auto itr = node->FindMember(key.c_str()); itr != node->MemberEnd()) {
      return &itr->value;
    }
    return nullptr;
  }
  NodeType addChild(NodeType node, const std::string& key) {
    // empty key means the node itself
    if (key.empty()) {
      return node;
    }
    node->AddMember(rapidjson::Value(key.c_str(), getAllocator()), rapidjson::Value(),
                    getAllocator());
    return &(*node)[key.c_str()];
  }

  bool isObject(NodeType node) { return node->IsObject(); }
  void setObject(NodeType node) { node->SetObject(); }

  bool isArray(NodeType node) { return node->IsArray(); }
  void setArray(NodeType node, size_t size) {
    node->SetArray();
    node->Reserve(size, getAllocator());
  }

  size_t getArraySize(NodeType node) { return node->Size(); }

  NodeType getArrayElement(NodeType node, size_t index) { return &(*node)[index]; }
  NodeType addArrayElement(NodeType node) {
    node->PushBack(rapidjson::Value(), getAllocator());
    return &(*node)[node->Size() - 1];
  }

  template <typename T>
  T getValue(NodeType node) {
    // rapidjson needs special handling for std::string
    if constexpr (std::is_same_v<T, std::string>) {
      return std::string(node->GetString());
    } else {
      return node->Get<T>();
    }
  }

  template <typename T>
  void setValue(NodeType node, T value) {
    if constexpr (std::is_same_v<T, std::string>) {
      node->SetString(value.c_str(), value.size(), getAllocator());
    } else {
      node->Set<T>(value);
    }
  }
};

// Convenience alias for JSON serializable types
template <typename T>
using JsonSerializable = Serializable<T, RapidJsonContext>;

}  // namespace serialization

// Global alias for easier access
template <typename T>
using JsonSerializable = serialization::JsonSerializable<T>;

}  // namespace lazy
