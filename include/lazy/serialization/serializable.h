#pragma once

#include <functional>
#include <istream>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

namespace lazy::serialization {

// Provides automatic serialization
template <typename ConcreteT, typename ContextT>
class Serializable {
 protected:
  // aliases to be used outside the class
  using ConcreteType = ConcreteT;
  using ContextType = ContextT;

  // field accessors
  using ConstFieldAccessor = std::function<const void*(const ConcreteType*)>;
  using MutableFieldAccessor = std::function<void*(ConcreteType*)>;

  // type erased functions for serialization and deserialization
  using SerializationFunction = std::function<void(
      const void*, ContextType&, typename ContextType::NodeType, const std::string&)>;
  using DeserializationFunction =
      std::function<void(void*, ContextType&, typename ContextType::NodeType, const std::string&)>;

  // field metadata
  struct SerializableField {
    SerializableField(std::string _name, ConstFieldAccessor _constAccessor,
                      MutableFieldAccessor _mutableAccessor, SerializationFunction _serializationFn,
                      DeserializationFunction _deserializationFn)
        : name(std::move(_name)),
          constAccessor(std::move(_constAccessor)),
          mutableAccessor(std::move(_mutableAccessor)),
          serializationFn(std::move(_serializationFn)),
          deserializationFn(std::move(_deserializationFn)) {}

    std::string name;
    ConstFieldAccessor constAccessor;
    MutableFieldAccessor mutableAccessor;
    SerializationFunction serializationFn;
    DeserializationFunction deserializationFn;
  };

  static std::vector<SerializableField> serializableFields_;

  // serialize all fields into the current node
  void serialize(ContextType& context, typename ContextType::NodeType node) const {
    for (auto& field : serializableFields_) {
      const void* fieldAddress = field.constAccessor(static_cast<const ConcreteType*>(this));
      field.serializationFn(fieldAddress, context, node, field.name);
    }
  }
  // deserialize all fields from the current node
  void deserialize(ContextType& context, typename ContextType::NodeType node) {
    for (auto& field : serializableFields_) {
      void* fieldAddress = field.mutableAccessor(static_cast<ConcreteType*>(this));
      field.deserializationFn(fieldAddress, context, node, field.name);
    }
  }

  // Serializer needs access to the two functions above
  template <typename ValueType, typename ContextType, typename Enable>
  friend struct Serializer;

 public:
  void serialize(std::ostream& stream) const {
    ContextType context;
    serialize(context, context.root());
    context.toStream(stream);
  }

  void deserialize(std::istream& stream) {
    ContextType context(stream);
    deserialize(context, context.root());
  }
};

template <typename ConcreteT, typename ContextT>
std::vector<typename Serializable<ConcreteT, ContextT>::SerializableField>
    Serializable<ConcreteT, ContextT>::serializableFields_;

// registers field metadata
// NOLINTBEGIN(cppcoreguidelines-macro-usage)
#define REGISTER_SERIALIZABLE_FIELD(type, name)                                                    \
  class register_##name##_helper {                                                                 \
    register_##name##_helper() {                                                                   \
      static std::once_flag registrationFlag;                                                      \
      std::call_once(registrationFlag, []() {                                                      \
        ConcreteType::serializableFields_.emplace_back(                                            \
            #name,                                                                                 \
            [](const ConcreteType* obj) -> const void* {                                           \
              return static_cast<const void*>(&obj->name);                                         \
            },                                                                                     \
            [](ConcreteType* obj) -> void* { return static_cast<void*>(&obj->name); },             \
            &lazy::serialization::Serializer<type, typename ConcreteType::ContextType>::serialize, \
            &lazy::serialization::Serializer<type,                                                 \
                                             typename ConcreteType::ContextType>::deserialize);    \
      });                                                                                          \
    }                                                                                              \
    friend ConcreteType;                                                                           \
  };                                                                                               \
  register_##name##_helper register_##name##_helper_;

// variadic macro implementation
#define SERIALIZABLE_FIELD_IMPL_2(type, name) \
 private:                                     \
  REGISTER_SERIALIZABLE_FIELD(type, name)     \
 public:                                      \
  type name{};

#define SERIALIZABLE_FIELD_IMPL_3(type, name, defaultValue) \
 private:                                                   \
  REGISTER_SERIALIZABLE_FIELD(type, name)                   \
 public:                                                    \
  type name = defaultValue;

// overload dispatcher
#define SERIALIZABLE_FIELD_GET_MACRO(_1, _2, _3, NAME, ...) NAME

// declare a serializable field
// !! note this can only be used inside Serializable class !!
#define SERIALIZABLE_FIELD(...)                                                                   \
  SERIALIZABLE_FIELD_GET_MACRO(__VA_ARGS__, SERIALIZABLE_FIELD_IMPL_3, SERIALIZABLE_FIELD_IMPL_2, \
                               unused)(__VA_ARGS__)
// NOLINTEND(cppcoreguidelines-macro-usage)

// serializer for primitive types
template <typename ValueType, typename ContextType, typename Enable = void>
struct Serializer {
  static void serialize(const void* value, ContextType& context,
                        typename ContextType::NodeType node, const std::string& key) {
    auto* childNode = context.addChild(node, key);
    context.template setValue<ValueType>(childNode, *static_cast<const ValueType*>(value));
  }
  static void deserialize(void* value, ContextType& context, typename ContextType::NodeType node,
                          const std::string& key) {
    auto* childNode = context.getChild(node, key);
    if (childNode) {
      *static_cast<ValueType*>(value) = context.template getValue<ValueType>(childNode);
    }
  }
};

// serializer for array
template <typename ValueType, typename ContextType>
struct Serializer<std::vector<ValueType>, ContextType> {
  static void serialize(const void* value, ContextType& context,
                        typename ContextType::NodeType node, const std::string& key) {
    const std::vector<ValueType>& vector = *static_cast<const std::vector<ValueType>*>(value);
    auto* childNode = context.addChild(node, key);
    context.setArray(childNode, vector.size());
    for (const auto& item : vector) {
      Serializer<ValueType, ContextType>::serialize(&item, context,
                                                    context.addArrayElement(childNode), "");
    }
  }

  static void deserialize(void* value, ContextType& context, typename ContextType::NodeType node,
                          const std::string& key) {
    auto* childNode = context.getChild(node, key);
    if (childNode && context.isArray(childNode)) {
      std::vector<ValueType>& vector = *static_cast<std::vector<ValueType>*>(value);
      vector.clear();
      vector.resize(context.getArraySize(childNode));
      for (size_t i = 0; i < vector.size(); ++i) {
        Serializer<ValueType, ContextType>::deserialize(&vector[i], context, childNode, "");
      }
    }
  }
};

// serializer for nested types
template <typename ValueType, typename ContextType>
struct Serializer<
    ValueType, ContextType,
    std::enable_if_t<std::is_base_of_v<Serializable<ValueType, ContextType>, ValueType>>> {
  static void serialize(const void* value, ContextType& context,
                        typename ContextType::NodeType node, const std::string& key) {
    auto* childNode = context.addChild(node, key);
    static_cast<const ValueType*>(value)->serialize(context, childNode);
  }
  static void deserialize(void* value, ContextType& context, typename ContextType::NodeType node,
                          const std::string& key) {
    auto* childNode = context.getChild(node, key);
    if (childNode) {
      static_cast<ValueType*>(value)->deserialize(context, childNode);
    }
  }
};

}  // namespace lazy::serialization
