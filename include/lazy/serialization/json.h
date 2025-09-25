#pragma once

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <string>
#include <type_traits>

namespace lazy::serialization {

// JSON serialization context using RapidJSON
class RapidJsonContext {
 public:
  using NodeType = rapidjson::Value*;

  // create a new / empty context to start serialization
  RapidJsonContext() { document_.SetObject(); }

  // load context fro deserialization
  explicit RapidJsonContext(const std::string& jsonString) { document_.Parse(jsonString.c_str()); }

  // serialize to JSON string
  std::string toString() const {
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    document_.Accept(writer);
    return buffer.GetString();
  }

  rapidjson::Document::AllocatorType& getAllocator() { return document_.GetAllocator(); }

  NodeType root() { return &document_; }
  NodeType getChild(NodeType node, const std::string& key) {
    if (key.empty()) {
      return node;
    }
    if (auto itr = node->FindMember(key.c_str()); itr != node->MemberEnd()) {
      return &itr->value;
    }
    return nullptr;
  }
  NodeType addChild(NodeType node, const std::string& key) {
    if (key.empty()) {
      return node;
    }
    node->AddMember(rapidjson::Value(key.c_str(), getAllocator()), rapidjson::Value(),
                    getAllocator());
    return &(*node)[key.c_str()];
  }

  bool isArray(NodeType node) { return node->IsArray(); }
  void setArray(NodeType node, size_t size) {
    node->SetArray();
    node->Reserve(size, getAllocator());
  }

  size_t getArraySize(NodeType node) { return node->Size(); }

  NodeType getArrayElement(NodeType node, size_t index) { return &(*node)[index]; }
  NodeType addArrayElement(NodeType node) {
    return &node->PushBack(rapidjson::Value(), getAllocator());
  }

  template <typename T>
  struct CustomValueTypes {
    constexpr static bool enabled = false;
    T getValue(RapidJsonContext& context, NodeType node) { return T(); }
    void setValue(RapidJsonContext& context, NodeType node, T value) {}
  };

  template <typename T>
  T getValue(NodeType node) {
    // if functor exists for T, use it
    if constexpr (CustomValueTypes<T>::enabled) {
      return CustomValueTypes<T>::getValue(*this, node);
    } else if constexpr (std::is_same_v<T, std::string>) {
      return std::string(node->GetString());
    } else {
      return node->Get<T>();
    }
  }

  template <typename T>
  void setValue(NodeType node, T value) {
    if constexpr (CustomValueTypes<T>::enabled) {
      CustomValueTypes<T>::setValue(*this, node, value);
    } else if constexpr (std::is_same_v<T, std::string>) {
      node->SetString(value.c_str(), value.size(), getAllocator());
    } else {
      node->Set<T>(value);
    }
  }

 private:
  rapidjson::Document document_;
};

}  // namespace lazy::serialization

#include "lazy/serialization/serializable.h"

namespace lazy {
namespace serialization {
template <typename T>
using JsonSerializable = Serializable<T, RapidJsonContext>;
}  // namespace serialization
template <typename T>
using JsonSerializable = serialization::JsonSerializable<T>;
}  // namespace lazy
