#pragma once

// Convenience include for JSON serialization
// Provides JsonAdapter and JsonSerializable aliases

#ifdef LAZY_SERIALIZATION_RAPID_JSON
// Use RapidJSON backend when explicitly enabled
#include "lazy/serialization/json_rapid.h"

namespace lazy {
namespace serialization {

// Use RapidJsonAdapter as the default JsonAdapter
using JsonAdapter = RapidJsonAdapter;
template <typename T>
using JsonSerializable = RapidJsonSerializable<T>;

}  // namespace serialization

// Global alias
template <typename T>
using JsonSerializable = lazy::serialization::JsonSerializable<T>;

}  // namespace lazy

#else
// Use built-in LazyJsonAdapter as default
#include "lazy/serialization/json_lazy.h"

namespace lazy {
namespace serialization {

// Use LazyJsonAdapter as the default JsonAdapter
using JsonAdapter = LazyJsonAdapter;
template <typename T>
using JsonSerializable = LazyJsonSerializable<T>;

}  // namespace serialization

// Global alias
template <typename T>
using JsonSerializable = lazy::serialization::JsonSerializable<T>;

}  // namespace lazy

#endif