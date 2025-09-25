#include <iostream>
#include <sstream>

/*
  Derive from lazy::serialization::Serializable<T, ContextType> to make a class serializable
  use SERIALIZABLE_FIELD to declare serializable fields

  Each serialization impl provides a convenience alias:
  - lazy::TextSerializable<T> = lazy::serialization::Serializable<T, TextContext>
  - lazy::JsonSerializable<T> = lazy::serialization::Serializable<T, JsonContext>
  - lazy::BinarySerializable<T> = lazy::serialization::Serializable<T, BinaryContext>
  - lazy::YamlSerializable<T> = lazy::serialization::Serializable<T, YamlContext>

  Example:
  #include "lazy/serialization/json.h"

  class MyClass : public lazy::JsonSerializable<MyClass> {
  public:
    SERIALIZABLE_FIELD(std::string, name, "MyClass");
  };

  MyClass myClass;
  myClass.serialize(stream);
  myClass.deserialize(stream);
*/

/*
  This example can be configured to use different serialization implementations.
  To select a format, compile with one of these flags:
  - -DUSE_TEXT (default): Text format (human-readable key-value pairs)
  - -DUSE_JSON: JSON format using RapidJSON
  - -DUSE_BINARY: Binary format (compact, endian-safe)
  - -DUSE_YAML: YAML format (human-readable, indented)
*/

// Define which serialization format to use
// Options: USE_TEXT, USE_JSON, USE_BINARY, USE_YAML
#ifndef USE_TEXT
#ifndef USE_JSON
#ifndef USE_BINARY
#ifndef USE_YAML
#define USE_TEXT  // Default to text format
#endif
#endif
#endif
#endif

#ifdef USE_TEXT
#include "lazy/serialization/text.h"
using SerializationContext = lazy::serialization::TextContext;
constexpr const char* kSerializationContextName = "Text";
#elif defined(USE_JSON)
#include "lazy/serialization/json.h"
using SerializationContext = lazy::serialization::RapidJsonContext;
constexpr const char* kSerializationContextName = "Json";
#elif defined(USE_BINARY)
#include "lazy/serialization/binary.h"
using SerializationContext = lazy::serialization::BinaryContext;
constexpr const char* kSerializationContextName = "Binary";
#elif defined(USE_YAML)
#include "lazy/serialization/yaml.h"
using SerializationContext = lazy::serialization::YamlContext;
constexpr const char* kSerializationContextName = "Yaml";
#endif

class MySubClass : public lazy::serialization::Serializable<MySubClass, SerializationContext> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "MySubClass");
};

class MySealedClass {
 public:
  std::string name = "MySealedClass";
};

namespace lazy::serialization {

// make MySealedClass serializable non-intrusively
SERIALIZABLE_TYPE(SerializationContext, MySealedClass, name)

}  // namespace lazy::serialization

class MyClass : public lazy::serialization::Serializable<MyClass, SerializationContext> {
 public:
  // Primitive fields with default value
  SERIALIZABLE_FIELD(std::string, name, "MyClass");
  // Primitive fields without default value
  SERIALIZABLE_FIELD(int, value);
  // Nested type
  SERIALIZABLE_FIELD(MySubClass, subClass);
  // External / sealed type
  SERIALIZABLE_FIELD(MySealedClass, sealedClass);
  // Vector types
  SERIALIZABLE_FIELD(std::vector<int>, intVector);
  SERIALIZABLE_FIELD(std::vector<MySubClass>, nestedVector);
  SERIALIZABLE_FIELD(std::vector<MySealedClass>, sealedVector);
};

int main() {
  std::cout << "Using serialization context: " << kSerializationContextName << "\n" << std::endl;

  MyClass myClass;

  std::stringstream ss;
  myClass.serialize(ss);
  std::cout << "Serialized Default: \n" << ss.str() << std::endl;

  myClass.name = "MyClass2";
  myClass.value = 42;
  myClass.subClass.name = "MySubClass2";
  myClass.sealedClass.name = "MySealedClass2";
  myClass.intVector = {1, 2, 3};

  std::stringstream ss2;
  myClass.serialize(ss2);
  std::cout << "Serialized Modified: \n" << ss2.str() << std::endl;

  MyClass myClass2;
  std::istringstream iss(ss2.str());
  myClass2.deserialize(iss);
  std::cout << "Deserialized Modified: \n"
            << "name: " << myClass2.name << "\n"
            << "value: " << myClass2.value << "\n"
            << "subClass.name: " << myClass2.subClass.name << "\n"
            << "sealedClass.name: " << myClass2.sealedClass.name << "\n"
            << "intVector.0: " << myClass2.intVector[0] << "\n"
            << "intVector.1: " << myClass2.intVector[1] << "\n"
            << "intVector.2: " << myClass2.intVector[2] << std::endl;

  return 0;
}
