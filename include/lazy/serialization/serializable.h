#pragma once

#include <functional>
#include <istream>
#include <mutex>
#include <ostream>
#include <string>
#include <vector>

namespace lazy::serialization {

/**
 * Provides automatic serialization
 *
 * ConcreteT: The derived class being made serializable
 * AdapterT: Serialization adapter (e.g., JSON, XML) providing node operations
 *
 * use SERIALIZABLE_FIELD to declare serializable fields
 *
 * Example:
 * class MyClass : public Serializable<MyClass, JsonAdapter> {
 *   SERIALIZABLE_FIELD(std::string, name, "MyClass");
 * };
 *
 * MyClass myClass;
 * myClass.serialize(stream);
 * myClass.deserialize(stream);
 */
template <typename ConcreteT, typename AdapterT>
class Serializable {
 private:
  struct SerializableFieldMetadata;

 protected:
  // Macros need these aliases
  using ConcreteType = ConcreteT;
  using SerializableAdapterType = AdapterT;

  static std::vector<SerializableFieldMetadata> serializationFields_;

 private:
  // Type-erased field accessors to get field addresses from object instances
  using FieldAccessor = std::function<const void*(const ConcreteType*)>;
  using MutableFieldAccessor = std::function<void*(ConcreteType*)>;

  // Type-erased serialization functions
  using SerializationFunction =
      std::function<void(const void*, SerializableAdapterType&,
                         typename SerializableAdapterType::NodeType, const std::string&)>;
  using DeserializationFunction =
      std::function<void(void*, SerializableAdapterType&,
                         typename SerializableAdapterType::NodeType, const std::string&)>;

  // Complete field metadata package
  struct SerializableFieldMetadata {
    SerializableFieldMetadata(std::string _name, FieldAccessor _constAccessor,
                              MutableFieldAccessor _mutableAccessor,
                              SerializationFunction _serializationFn,
                              DeserializationFunction _deserializationFn)
        : name(std::move(_name)),
          constAccessor(std::move(_constAccessor)),
          mutableAccessor(std::move(_mutableAccessor)),
          serializationFn(std::move(_serializationFn)),
          deserializationFn(std::move(_deserializationFn)) {}

    std::string name;
    FieldAccessor constAccessor;
    MutableFieldAccessor mutableAccessor;
    SerializationFunction serializationFn;
    DeserializationFunction deserializationFn;
  };

  // Core serialization - iterate through registered fields
  void serialize(SerializableAdapterType& adapter,
                 typename SerializableAdapterType::NodeType node) const {
    for (auto& field : serializationFields_) {
      const void* fieldAddress = field.constAccessor(static_cast<const ConcreteType*>(this));
      field.serializationFn(fieldAddress, adapter, node, field.name);
    }
  }

  void deserialize(SerializableAdapterType& adapter,
                   typename SerializableAdapterType::NodeType node) {
    for (auto& field : serializationFields_) {
      void* fieldAddress = field.mutableAccessor(static_cast<ConcreteType*>(this));
      field.deserializationFn(fieldAddress, adapter, node, field.name);
    }
  }

  // Serializer needs access to the two functions above
  template <typename ValueType, typename AdapterType, typename Enable>
  friend struct Serializer;

 public:
  void serialize(std::ostream& stream) const {
    SerializableAdapterType adapter(stream);
    serialize(adapter, adapter.root());
    adapter.finishSerialization();
  }

  void deserialize(std::istream& stream) {
    SerializableAdapterType adapter(stream);
    deserialize(adapter, adapter.root());
    adapter.finishDeserialization();
  }
};

template <typename ConcreteT, typename AdapterT>
std::vector<typename Serializable<ConcreteT, AdapterT>::SerializableFieldMetadata>
    Serializable<ConcreteT, AdapterT>::serializationFields_;

// ================================================================================================
// SERIALIZABLE_FIELD for declaring serializable fields
//
// Example:
// SERIALIZABLE_FIELD(std::string, name, "MyClass");
// SERIALIZABLE_FIELD(int, value);
// SERIALIZABLE_FIELD(MySubClass, subClass);
// ================================================================================================

/**
 * Field registration via static initialization.
 * Each field is registered exactly once.
 */
#define REGISTER_SERIALIZABLE_FIELD(type, name)                                            \
  class register_##name##_helper {                                                         \
    register_##name##_helper() {                                                           \
      static std::once_flag registrationFlag;                                              \
      std::call_once(registrationFlag, []() {                                              \
        ConcreteType::serializationFields_.emplace_back(                                   \
            #name,                                                                         \
            [](const ConcreteType* obj) -> const void* {                                   \
              return static_cast<const void*>(&obj->name);                                 \
            },                                                                             \
            [](ConcreteType* obj) -> void* { return static_cast<void*>(&obj->name); },     \
            &lazy::serialization::Serializer<type, SerializableAdapterType>::serialize,    \
            &lazy::serialization::Serializer<type, SerializableAdapterType>::deserialize); \
      });                                                                                  \
    }                                                                                      \
    friend ConcreteType;                                                                   \
  };                                                                                       \
  register_##name##_helper register_##name##_helper_;

// SerializableField macro implementation without default value
#define SERIALIZABLE_FIELD_IMPL_2(type, name) \
 private:                                     \
  REGISTER_SERIALIZABLE_FIELD(type, name)     \
 public:                                      \
  type name{};

// SerializableField macro implementation with default value
#define SERIALIZABLE_FIELD_IMPL_3(type, name, defaultValue) \
 private:                                                   \
  REGISTER_SERIALIZABLE_FIELD(type, name)                   \
 public:                                                    \
  type name = defaultValue;

// Macro overload selection based on argument count
#define SERIALIZABLE_FIELD_GET_MACRO(_1, _2, _3, NAME, ...) NAME

/**
 * Declare a serializable field
 * Note: this needs to be used inside Serializable class
 *
 * Example:
 * SERIALIZABLE_FIELD(std::string, name, "MyClass");
 * SERIALIZABLE_FIELD(int, value);
 * SERIALIZABLE_FIELD(MySubClass, subClass);
 */
#define SERIALIZABLE_FIELD(...)                                                                   \
  SERIALIZABLE_FIELD_GET_MACRO(__VA_ARGS__, SERIALIZABLE_FIELD_IMPL_3, SERIALIZABLE_FIELD_IMPL_2, \
                               unused)(__VA_ARGS__)

// ================================================================================================
// Serializer
//
// Handles:
// 1. Primitive types (int, float, string, etc.)
// 2. Array types (std::vector<T>)
// 3. Object types (Serializable-derived types)
// 4. Custom types (SERIALIZABLE_TYPE, see next section)
// ================================================================================================

// Primitive types that can be handled directly by the specific serializer context
template <typename ValueType, typename AdapterType, typename Enable = void>
struct Serializer {
  static void serialize(const void* value, AdapterType& adapter,
                        typename AdapterType::NodeType node, const std::string& key) {
    auto* childNode = adapter.addChild(node, key);
    adapter.template setValue<ValueType>(childNode, *static_cast<const ValueType*>(value));
  }
  static void deserialize(void* value, AdapterType& adapter, typename AdapterType::NodeType node,
                          const std::string& key) {
    auto* childNode = adapter.getChild(node, key);
    if (childNode) {
      *static_cast<ValueType*>(value) = adapter.template getValue<ValueType>(childNode);
    }
  }
};

// Unpack array and forward handling to Serializer<ElementType>
template <typename ValueType, typename AdapterType>
struct Serializer<std::vector<ValueType>, AdapterType> {
  static void serialize(const void* value, AdapterType& adapter,
                        typename AdapterType::NodeType node, const std::string& key) {
    const std::vector<ValueType>& vector = *static_cast<const std::vector<ValueType>*>(value);
    auto* childNode = adapter.addChild(node, key);
    adapter.setArray(childNode, vector.size());
    for (const auto& item : vector) {
      Serializer<ValueType, AdapterType>::serialize(&item, adapter,
                                                    adapter.addArrayElement(childNode), "");
    }
  }

  static void deserialize(void* value, AdapterType& adapter, typename AdapterType::NodeType node,
                          const std::string& key) {
    auto* childNode = adapter.getChild(node, key);
    if (childNode && adapter.isArray(childNode)) {
      std::vector<ValueType>& vector = *static_cast<std::vector<ValueType>*>(value);
      vector.clear();
      vector.resize(adapter.getArraySize(childNode));
      for (size_t i = 0; i < vector.size(); ++i) {
        auto* elementNode = adapter.getArrayElement(childNode, i);
        Serializer<ValueType, AdapterType>::deserialize(&vector[i], adapter, elementNode, "");
      }
    }
  }
};

// Nested types derived from Serializable
template <typename ValueType, typename AdapterType>
struct Serializer<
    ValueType, AdapterType,
    std::enable_if_t<std::is_base_of_v<Serializable<ValueType, AdapterType>, ValueType>>> {
  static void serialize(const void* value, AdapterType& adapter,
                        typename AdapterType::NodeType node, const std::string& key) {
    auto* childNode = adapter.addChild(node, key);
    adapter.setObject(childNode);
    static_cast<const ValueType*>(value)->serialize(adapter, childNode);
  }
  static void deserialize(void* value, AdapterType& adapter, typename AdapterType::NodeType node,
                          const std::string& key) {
    auto* childNode = adapter.getChild(node, key);
    if (childNode && adapter.isObject(childNode)) {
      static_cast<ValueType*>(value)->deserialize(adapter, childNode);
    }
  }
};

// ================================================================================================
// SERIALIZABLE_TYPE to handle custom types
//
// Add Serializer specializations for external/sealed classes.
//
// Example:
// SERIALIZABLE_TYPE(JsonAdapter, MySealedClass, name);
// ================================================================================================

// --- Variadic Macro Utilities ---

// Variadic FOR_EACH implementation - applies macro to each argument
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

// --- Field Serialization Helpers ---

// Helper to generate deserialize body for a single field
#define SERIALIZABLE_TYPE_DESERIALIZE_FIELD(field_name)                                     \
  if (auto field_name##Node = adapter.getChild(childNode, #field_name); field_name##Node) { \
    result->field_name =                                                                    \
        adapter.template getValue<decltype(result->field_name)>(field_name##Node);          \
  }

// Helper to generate serialize body for a single field
#define SERIALIZABLE_TYPE_SERIALIZE_FIELD(field_name)                                        \
  {                                                                                          \
    auto field_name##Node = adapter.addChild(childNode, #field_name);                        \
    adapter.template setValue<decltype(obj->field_name)>(field_name##Node, obj->field_name); \
  }

// --- Main Macro Implementation ---

// Single unified macro implementation - no more duplication!
// Handles any number of fields using variadic FOR_EACH
#define SERIALIZABLE_TYPE_IMPL(AdapterType, Type, ...)                                     \
  template <>                                                                              \
  struct Serializer<Type, AdapterType> {                                                   \
    static void serialize(const void* value, AdapterType& adapter,                         \
                          typename AdapterType::NodeType node, const std::string& key) {   \
      const Type* obj = static_cast<const Type*>(value);                                   \
      auto childNode = adapter.addChild(node, key);                                        \
      adapter.setObject(childNode);                                                        \
      SERIALIZABLE_FOR_EACH(SERIALIZABLE_TYPE_SERIALIZE_FIELD, __VA_ARGS__)                \
    }                                                                                      \
    static void deserialize(void* value, AdapterType& adapter,                             \
                            typename AdapterType::NodeType node, const std::string& key) { \
      Type* result = static_cast<Type*>(value);                                            \
      auto childNode = adapter.getChild(node, key);                                        \
      if (childNode && adapter.isObject(childNode)) {                                      \
        SERIALIZABLE_FOR_EACH(SERIALIZABLE_TYPE_DESERIALIZE_FIELD, __VA_ARGS__)            \
      }                                                                                    \
    }                                                                                      \
  };

/**
 * Add Serializer specializations for external/sealed classes.
 *
 * Example:
 * namespace lazy::serialization {
 *   SERIALIZABLE_TYPE(JsonAdapter, MySealedClass, field1, field2, ...);
 * }
 */
#define SERIALIZABLE_TYPE(AdapterType, Type, ...) \
  SERIALIZABLE_TYPE_IMPL(AdapterType, Type, __VA_ARGS__)

}  // namespace lazy::serialization
