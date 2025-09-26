#pragma once

#include <functional>
#include <istream>
#include <mutex>
#include <ostream>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "lazy/serialization/binary.h"
#include "lazy/serialization/json_lazy.h"
#include "lazy/serialization/serializable.h"
#include "lazy/serialization/text.h"

#ifdef LAZY_SERIALIZATION_RAPID_JSON
#include "lazy/serialization/json_rapid.h"
#endif
#ifdef LAZY_SERIALIZATION_YAML_ENABLED
#include "lazy/serialization/yaml.h"
#endif

namespace lazy::serialization {

// ================================================================================================
// RUNTIME TYPE DISPATCH REGISTRY
// ================================================================================================

template <typename AdapterType>
class TypeDispatchRegistry {
 private:
  using SerializeFunction = std::function<void(const void*, AdapterType&,
                                               typename AdapterType::NodeType, const std::string&)>;
  using DeserializeFunction =
      std::function<void(void*, AdapterType&, typename AdapterType::NodeType, const std::string&)>;

  static std::unordered_map<std::type_index, SerializeFunction> serializers_;
  static std::unordered_map<std::type_index, DeserializeFunction> deserializers_;
  static std::mutex registryMutex_;

 public:
  template <typename T>
  static void registerType() {
    std::lock_guard<std::mutex> lock(registryMutex_);

    std::type_index typeIdx = std::type_index(typeid(T));

    // Register serializer
    serializers_[typeIdx] = [](const void* value, AdapterType& adapter,
                               typename AdapterType::NodeType node, const std::string& key) {
      Serializer<T, AdapterType>::serialize(value, adapter, node, key);
    };

    // Register deserializer
    deserializers_[typeIdx] = [](void* value, AdapterType& adapter,
                                 typename AdapterType::NodeType node, const std::string& key) {
      Serializer<T, AdapterType>::deserialize(value, adapter, node, key);
    };
  }

  static void serialize(std::type_index typeIndex, const void* value, AdapterType& adapter,
                        typename AdapterType::NodeType node, const std::string& key) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = serializers_.find(typeIndex);
    if (it != serializers_.end()) {
      it->second(value, adapter, node, key);
    }
  }

  static void deserialize(std::type_index typeIndex, void* value, AdapterType& adapter,
                          typename AdapterType::NodeType node, const std::string& key) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    auto it = deserializers_.find(typeIndex);
    if (it != deserializers_.end()) {
      it->second(value, adapter, node, key);
    }
  }

  static bool hasSerializer(std::type_index typeIndex) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    return serializers_.find(typeIndex) != serializers_.end();
  }

  static bool hasDeserializer(std::type_index typeIndex) {
    std::lock_guard<std::mutex> lock(registryMutex_);
    return deserializers_.find(typeIndex) != deserializers_.end();
  }
};

// Static member definitions
template <typename AdapterType>
std::unordered_map<std::type_index, typename TypeDispatchRegistry<AdapterType>::SerializeFunction>
    TypeDispatchRegistry<AdapterType>::serializers_;

template <typename AdapterType>
std::unordered_map<std::type_index, typename TypeDispatchRegistry<AdapterType>::DeserializeFunction>
    TypeDispatchRegistry<AdapterType>::deserializers_;

template <typename AdapterType>
std::mutex TypeDispatchRegistry<AdapterType>::registryMutex_;

// Register a type with all supported adapters
template <typename T>
void registerTypeWithAllAdapters() {
  static std::once_flag flag;
  std::call_once(flag, []() {
    TypeDispatchRegistry<TextAdapter>::template registerType<T>();
    TypeDispatchRegistry<BinaryAdapter>::template registerType<T>();
    TypeDispatchRegistry<LazyJsonAdapter>::template registerType<T>();

#ifdef LAZY_SERIALIZATION_RAPID_JSON
    TypeDispatchRegistry<RapidJsonAdapter>::template registerType<T>();
#endif
#ifdef LAZY_SERIALIZATION_YAML_ENABLED
    TypeDispatchRegistry<YamlAdapter>::template registerType<T>();
#endif
  });
}

// ================================================================================================
// MULTI_SERIALIZABLE BASE CLASS
// ================================================================================================

/**
 * MultiSerializable allows a single class definition to work with multiple serialization adapters.
 * Instead of being tied to a specific adapter at class definition time, the adapter is selected
 * at usage time through template parameters.
 *
 * Standard supported adapters (always available):
 * - TextAdapter, BinaryAdapter, LazyJsonAdapter
 *
 * Optional adapters (enable via preprocessor flags):
 * - RapidJsonAdapter (requires: #define LAZY_SERIALIZATION_RAPID_JSON)
 * - YamlAdapter (requires: #define LAZY_SERIALIZATION_YAML_ENABLED)
 *
 * Example:
 * class MyClass : public MultiSerializable<MyClass> {
 *   MULTI_SERIALIZABLE_FIELD(std::string, name, "MyClass");
 *   MULTI_SERIALIZABLE_FIELD(int, value, 42);
 * };
 *
 * // Usage with different adapters:
 * MyClass obj;
 * obj.serialize<TextAdapter>(stream);      // Always available
 * obj.serialize<BinaryAdapter>(stream);    // Always available
 * obj.serialize<LazyJsonAdapter>(stream);  // Always available
 * obj.serialize<RapidJsonAdapter>(stream); // If LAZY_SERIALIZATION_RAPID_JSON defined
 * obj.serialize<YamlAdapter>(stream);      // If LAZY_SERIALIZATION_YAML_ENABLED defined
 */
template <typename ConcreteT>
class MultiSerializable {
 protected:
  using ConcreteType = ConcreteT;

 private:
  // Type-erased field accessors
  using FieldAccessor = std::function<const void*(const ConcreteType*)>;
  using MutableFieldAccessor = std::function<void*(ConcreteType*)>;

  // Field metadata that stores type information for runtime dispatch
  struct FieldMetadata {
    std::string name;
    std::type_index typeIndex;
    FieldAccessor constAccessor;
    MutableFieldAccessor mutableAccessor;

    FieldMetadata(std::string _name, std::type_index _typeIndex, FieldAccessor _constAccessor,
                  MutableFieldAccessor _mutableAccessor)
        : name(std::move(_name)),
          typeIndex(_typeIndex),
          constAccessor(std::move(_constAccessor)),
          mutableAccessor(std::move(_mutableAccessor)) {}
  };

 public:
  // Template serialization core - works with any adapter (public for nested access)
  template <typename AdapterType>
  void serialize(AdapterType& adapter, typename AdapterType::NodeType node) const {
    for (const auto& field : multiSerializationFields_) {
      const void* fieldAddress = field.constAccessor(static_cast<const ConcreteType*>(this));
      TypeDispatchRegistry<AdapterType>::serialize(field.typeIndex, fieldAddress, adapter, node,
                                                   field.name);
    }
  }

  template <typename AdapterType>
  void deserialize(AdapterType& adapter, typename AdapterType::NodeType node) {
    for (const auto& field : multiSerializationFields_) {
      void* fieldAddress = field.mutableAccessor(static_cast<ConcreteType*>(this));
      TypeDispatchRegistry<AdapterType>::deserialize(field.typeIndex, fieldAddress, adapter, node,
                                                     field.name);
    }
  }

 protected:
  static std::vector<FieldMetadata> multiSerializationFields_;

 public:
  // Template serialize function that works with any adapter
  template <typename AdapterType>
  void serialize(std::ostream& stream) const {
    AdapterType adapter(stream);
    serialize(adapter, adapter.root());
    adapter.finishSerialization();
  }

  template <typename AdapterType>
  void deserialize(std::istream& stream) {
    AdapterType adapter(stream);
    deserialize(adapter, adapter.root());
  }
};

// Static member definition
template <typename ConcreteT>
std::vector<typename MultiSerializable<ConcreteT>::FieldMetadata>
    MultiSerializable<ConcreteT>::multiSerializationFields_;

// ================================================================================================
// CONVENIENCE MACROS FOR EXTERNAL TYPES
// ================================================================================================

/**
 * MULTI_SERIALIZABLE_TYPE - Make an external/sealed class work with ALL MultiSerializable adapters
 *
 * This macro:
 * 1. Creates SERIALIZABLE_TYPE specializations for all supported adapters
 * 2. Provides a convenient registration function
 *
 * Usage:
 * class Product {
 * public:
 *   std::string name;
 *   double price;
 * };
 *
 * MULTI_SERIALIZABLE_TYPE(Product, name, price)
 *
 * // Then call once in your main function:
 * REGISTER_MULTI_SERIALIZABLE_TYPE(Product);
 *
 * Now Product works with TextAdapter, BinaryAdapter, LazyJsonAdapter!
 */
#define MULTI_SERIALIZABLE_TYPE(Type, ...)                                   \
  /* Create SERIALIZABLE_TYPE specializations for all standard adapters */   \
  SERIALIZABLE_TYPE(lazy::serialization::TextAdapter, Type, __VA_ARGS__)     \
  SERIALIZABLE_TYPE(lazy::serialization::BinaryAdapter, Type, __VA_ARGS__)   \
  SERIALIZABLE_TYPE(lazy::serialization::LazyJsonAdapter, Type, __VA_ARGS__) \
  /* Optional adapters - only if enabled */                                  \
  LAZY_SERIALIZATION_RAPID_JSON_SERIALIZABLE_TYPE(Type, __VA_ARGS__)         \
  LAZY_SERIALIZATION_YAML_SERIALIZABLE_TYPE(Type, __VA_ARGS__)

/**
 * REGISTER_MULTI_SERIALIZABLE_TYPE - Register a type created with MULTI_SERIALIZABLE_TYPE
 *
 * Call this once (e.g., in main()) for each MULTI_SERIALIZABLE_TYPE you want to use.
 * This registers the type with all available adapters.
 */
#define REGISTER_MULTI_SERIALIZABLE_TYPE(Type)                                          \
  do {                                                                                  \
    static std::once_flag Type##_register_flag;                                         \
    std::call_once(Type##_register_flag,                                                \
                   []() { lazy::serialization::registerTypeWithAllAdapters<Type>(); }); \
  } while (0)

// Helper macros for optional adapters (expand only if enabled)
#ifdef LAZY_SERIALIZATION_RAPID_JSON
#define LAZY_SERIALIZATION_RAPID_JSON_SERIALIZABLE_TYPE(Type, ...) \
  SERIALIZABLE_TYPE(lazy::serialization::RapidJsonAdapter, Type, __VA_ARGS__)
#else
#define LAZY_SERIALIZATION_RAPID_JSON_SERIALIZABLE_TYPE(Type, ...)
#endif

#ifdef LAZY_SERIALIZATION_YAML_ENABLED
#define LAZY_SERIALIZATION_YAML_SERIALIZABLE_TYPE(Type, ...) \
  SERIALIZABLE_TYPE(lazy::serialization::YamlAdapter, Type, __VA_ARGS__)
#else
#define LAZY_SERIALIZATION_YAML_SERIALIZABLE_TYPE(Type, ...)
#endif

// Field registration for MultiSerializable
#define REGISTER_MULTI_SERIALIZABLE_FIELD(type, name)                                   \
  class register_multi_##name##_helper {                                                \
    register_multi_##name##_helper() {                                                  \
      static std::once_flag registrationFlag;                                           \
      std::call_once(registrationFlag, []() {                                           \
        ConcreteType::multiSerializationFields_.emplace_back(                           \
            #name, std::type_index(typeid(type)),                                       \
            [](const ConcreteType* obj) -> const void* {                                \
              return static_cast<const void*>(&obj->name);                              \
            },                                                                          \
            [](ConcreteType* obj) -> void* { return static_cast<void*>(&obj->name); }); \
        /* Only register non-MultiSerializable types to avoid static init issues */     \
        if constexpr (!std::is_base_of_v<MultiSerializable<type>, type>) {              \
          lazy::serialization::registerTypeWithAllAdapters<type>();                     \
        }                                                                               \
      });                                                                               \
    }                                                                                   \
    friend ConcreteType;                                                                \
  };                                                                                    \
  register_multi_##name##_helper register_multi_##name##_helper_;

// Field macro implementations
#define MULTI_SERIALIZABLE_FIELD_IMPL_2(type, name) \
 private:                                           \
  REGISTER_MULTI_SERIALIZABLE_FIELD(type, name)     \
 public:                                            \
  type name{};

#define MULTI_SERIALIZABLE_FIELD_IMPL_3(type, name, defaultValue) \
 private:                                                         \
  REGISTER_MULTI_SERIALIZABLE_FIELD(type, name)                   \
 public:                                                          \
  type name = defaultValue;

#define MULTI_SERIALIZABLE_FIELD_GET_MACRO(_1, _2, _3, NAME, ...) NAME

/**
 * Declare a field that can be serialized with any adapter
 *
 * Example:
 * MULTI_SERIALIZABLE_FIELD(std::string, name, "MyClass");
 * MULTI_SERIALIZABLE_FIELD(int, value);
 */
#define MULTI_SERIALIZABLE_FIELD(...)                                              \
  MULTI_SERIALIZABLE_FIELD_GET_MACRO(__VA_ARGS__, MULTI_SERIALIZABLE_FIELD_IMPL_3, \
                                     MULTI_SERIALIZABLE_FIELD_IMPL_2, unused)(__VA_ARGS__)

// ================================================================================================
// TEMPLATE SPECIALIZATIONS FOR NESTED TYPES
// ================================================================================================

// Template specialization for MultiSerializable types (handles nested objects)
template <typename T, typename AdapterType>
struct Serializer<T, AdapterType, std::enable_if_t<std::is_base_of_v<MultiSerializable<T>, T>>> {
  static void serialize(const void* value, AdapterType& adapter,
                        typename AdapterType::NodeType node, const std::string& key) {
    // Ensure the nested type is registered (avoids static initialization issues)
    static std::once_flag flag;
    std::call_once(flag, []() { registerTypeWithAllAdapters<T>(); });

    const T* obj = static_cast<const T*>(value);
    // Create a sub-node for the nested object using adapter interface
    auto childNode = adapter.addChild(node, key);
    adapter.setObject(childNode);
    obj->template serialize<AdapterType>(adapter, childNode);
  }

  static void deserialize(void* value, AdapterType& adapter, typename AdapterType::NodeType node,
                          const std::string& key) {
    // Ensure the nested type is registered (avoids static initialization issues)
    static std::once_flag flag;
    std::call_once(flag, []() { registerTypeWithAllAdapters<T>(); });

    T* obj = static_cast<T*>(value);
    auto childNode = adapter.getChild(node, key);
    if (childNode && adapter.isObject(childNode)) {
      obj->template deserialize<AdapterType>(adapter, childNode);
    }
  }
};

}  // namespace lazy::serialization
