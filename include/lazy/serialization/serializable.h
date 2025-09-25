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
    context.setObject(childNode);
    static_cast<const ValueType*>(value)->serialize(context, childNode);
  }
  static void deserialize(void* value, ContextType& context, typename ContextType::NodeType node,
                          const std::string& key) {
    auto* childNode = context.getChild(node, key);
    if (childNode && context.isObject(childNode)) {
      static_cast<ValueType*>(value)->deserialize(context, childNode);
    }
  }
};

// Generic macros for generating Serializer specializations for custom types

// Variadic FOR_EACH implementation - applies a macro to each argument
#define SERIALIZABLE_APPLY_0(m, ...)
#define SERIALIZABLE_APPLY_1(m, x1) m(x1)
#define SERIALIZABLE_APPLY_2(m, x1, x2) m(x1) m(x2)
#define SERIALIZABLE_APPLY_3(m, x1, x2, x3) m(x1) m(x2) m(x3)
#define SERIALIZABLE_APPLY_4(m, x1, x2, x3, x4) m(x1) m(x2) m(x3) m(x4)
#define SERIALIZABLE_APPLY_5(m, x1, x2, x3, x4, x5) m(x1) m(x2) m(x3) m(x4) m(x5)
#define SERIALIZABLE_APPLY_6(m, x1, x2, x3, x4, x5, x6) m(x1) m(x2) m(x3) m(x4) m(x5) m(x6)
#define SERIALIZABLE_APPLY_7(m, x1, x2, x3, x4, x5, x6, x7) \
  m(x1) m(x2) m(x3) m(x4) m(x5) m(x6) m(x7)
#define SERIALIZABLE_APPLY_8(m, x1, x2, x3, x4, x5, x6, x7, x8) \
  m(x1) m(x2) m(x3) m(x4) m(x5) m(x6) m(x7) m(x8)

#define SERIALIZABLE_NARGS_IMPL(_1, _2, _3, _4, _5, _6, _7, _8, N, ...) N
#define SERIALIZABLE_NARGS(...) SERIALIZABLE_NARGS_IMPL(__VA_ARGS__, 8, 7, 6, 5, 4, 3, 2, 1, 0)

#define SERIALIZABLE_APPLY_CONCAT(a, b) a##b
#define SERIALIZABLE_APPLY_IMPL(n, m, ...) \
  SERIALIZABLE_APPLY_CONCAT(SERIALIZABLE_APPLY_, n)(m, __VA_ARGS__)
#define SERIALIZABLE_FOR_EACH(m, ...) \
  SERIALIZABLE_APPLY_IMPL(SERIALIZABLE_NARGS(__VA_ARGS__), m, __VA_ARGS__)

// Helper to generate deserialize body for a single field
#define SERIALIZABLE_TYPE_DESERIALIZE_FIELD(field_name)                                     \
  if (auto field_name##Node = context.getChild(childNode, #field_name); field_name##Node) { \
    result->field_name =                                                                    \
        context.template getValue<decltype(result->field_name)>(field_name##Node);          \
  }

// Helper to generate serialize body for a single field
#define SERIALIZABLE_TYPE_SERIALIZE_FIELD(field_name)                                        \
  {                                                                                          \
    auto field_name##Node = context.addChild(childNode, #field_name);                        \
    context.template setValue<decltype(obj->field_name)>(field_name##Node, obj->field_name); \
  }

// Single unified macro implementation - no more duplication!
// Handles any number of fields using variadic FOR_EACH
#define SERIALIZABLE_TYPE_IMPL(ContextType, Type, ...)                                     \
  template <>                                                                              \
  struct Serializer<Type, ContextType> {                                                   \
    static void serialize(const void* value, ContextType& context,                         \
                          typename ContextType::NodeType node, const std::string& key) {   \
      const Type* obj = static_cast<const Type*>(value);                                   \
      auto childNode = context.addChild(node, key);                                        \
      context.setObject(childNode);                                                        \
      SERIALIZABLE_FOR_EACH(SERIALIZABLE_TYPE_SERIALIZE_FIELD, __VA_ARGS__)                \
    }                                                                                      \
    static void deserialize(void* value, ContextType& context,                             \
                            typename ContextType::NodeType node, const std::string& key) { \
      Type* result = static_cast<Type*>(value);                                            \
      auto childNode = context.getChild(node, key);                                        \
      if (childNode && context.isObject(childNode)) {                                      \
        SERIALIZABLE_FOR_EACH(SERIALIZABLE_TYPE_DESERIALIZE_FIELD, __VA_ARGS__)            \
      }                                                                                    \
    }                                                                                      \
  };

// Main macro - simply delegates to the unified implementation
// Usage: SERIALIZABLE_TYPE(ContextType, Type, field1, field2, ...)
// Supports up to 8 fields (can be easily extended by adding more SERIALIZABLE_APPLY_N macros)
#define SERIALIZABLE_TYPE(ContextType, Type, ...) \
  SERIALIZABLE_TYPE_IMPL(ContextType, Type, __VA_ARGS__)

}  // namespace lazy::serialization
