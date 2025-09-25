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

// JSON serialization context using RapidJSON
class RapidJsonContext {
 public:
  using NodeType = rapidjson::Value*;

  RapidJsonContext() { document_.SetObject(); }

  explicit RapidJsonContext(std::istream& stream) {
    rapidjson::IStreamWrapper isw(stream);
    document_.ParseStream(isw);
  }

  void toStream(std::ostream& stream) const {
    rapidjson::OStreamWrapper osw(stream);
    rapidjson::Writer<rapidjson::OStreamWrapper> writer(osw);
    document_.Accept(writer);
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

template <typename T>
using JsonSerializable = Serializable<T, RapidJsonContext>;

}  // namespace serialization

template <typename T>
using JsonSerializable = serialization::JsonSerializable<T>;

}  // namespace lazy
