#include <iostream>
#include <sstream>

#include "lazy/serialization/multi_serializable.h"
#include "lazy/serialization/text.h"

using namespace lazy::serialization;

// ================================================================================================
// PART 1: TRADITIONAL SERIALIZABLE (Fixed adapter at compile time)
// ================================================================================================

class SimpleClass : public TextSerializable<SimpleClass> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "SimpleClass");
  SERIALIZABLE_FIELD(int, id, 100);
};

// External/sealed class example
class SealedClass {
 public:
  std::string category = "sealed";
  double value = 3.14;
};

// Make SealedClass work with TextAdapter
namespace lazy::serialization {
SERIALIZABLE_TYPE(TextAdapter, SealedClass, category, value)
}

class ComplexClass : public TextSerializable<ComplexClass> {
 public:
  SERIALIZABLE_FIELD(std::string, title, "Demo");
  SERIALIZABLE_FIELD(int, count, 0);
  SERIALIZABLE_FIELD(std::vector<std::string>, tags);
  SERIALIZABLE_FIELD(SimpleClass, nested);
  SERIALIZABLE_FIELD(SealedClass, sealed);
};

void serializableExample() {
  std::cout << "\n================================================" << std::endl;
  std::cout << "\n🔧 PART 1: Traditional Serializable (TextAdapter)" << std::endl;

  ComplexClass demo;
  demo.title = "Traditional Example";
  demo.count = 42;
  demo.tags = {"example", "serializable", "demo"};
  demo.nested.name = "Nested Object";
  demo.nested.id = 123;
  demo.sealed.category = "important";
  demo.sealed.value = 99.9;

  // Serialize
  std::cout << "\n📤 Serialization:" << std::endl;
  std::ostringstream serialOutput;
  demo.serialize(serialOutput);
  std::cout << serialOutput.str() << std::endl;

  // Round-trip test
  std::cout << "🔄 Round-trip Test:" << std::endl;
  ComplexClass deserialized;
  std::istringstream deserialInput(serialOutput.str());
  deserialized.deserialize(deserialInput);

  std::cout << "✅ Deserialized: title=" << deserialized.title << ", count=" << deserialized.count
            << ", nested.name=" << deserialized.nested.name << std::endl;
}

// ================================================================================================
// PART 2: MULTI-SERIALIZABLE (Choose adapter at runtime)
// ================================================================================================

class MultiSimpleClass : public MultiSerializable<MultiSimpleClass> {
 public:
  MULTI_SERIALIZABLE_FIELD(std::string, name, "MultiSimpleClass");
  MULTI_SERIALIZABLE_FIELD(int, id, 100);
};

class MultiComplexClass : public MultiSerializable<MultiComplexClass> {
 public:
  MULTI_SERIALIZABLE_FIELD(std::string, name, "MultiDemo");
  MULTI_SERIALIZABLE_FIELD(int, score, 85);
  MULTI_SERIALIZABLE_FIELD(std::vector<int>, numbers);
  MULTI_SERIALIZABLE_FIELD(MultiSimpleClass, nested);
};

void multiSerializableExample() {
  std::cout << "\n================================================" << std::endl;
  std::cout << "\n🚀 PART 2: MultiSerializable (Multiple Adapters)" << std::endl;

  MultiComplexClass multi;
  multi.name = "Multi Example";
  multi.score = 95; 
  multi.numbers = {10, 20, 30};
  multi.nested.name = "Nested Object";
  multi.nested.id = 200;

  // Serialize with TextAdapter 
  std::cout << "\n📝 TextAdapter:" << std::endl;
  std::ostringstream textStream;
  multi.serialize<TextAdapter>(textStream);
  std::cout << textStream.str() << std::endl;

  // Serialize with LazyJsonAdapter
  std::cout << "🔧 LazyJsonAdapter:" << std::endl;
  std::ostringstream jsonStream;
  multi.serialize<LazyJsonAdapter>(jsonStream);
  std::cout << jsonStream.str() << std::endl;

  // Round-trip test with JSON
  std::cout << "🔄 JSON Round-trip Test:" << std::endl;
  MultiComplexClass jsonDeserialized;
  std::istringstream jsonInput(jsonStream.str());
  jsonDeserialized.deserialize<LazyJsonAdapter>(jsonInput);

  std::cout << "✅ Deserialized: name=" << jsonDeserialized.name
            << ", score=" << jsonDeserialized.score
            << ", nested.id=" << jsonDeserialized.nested.id << std::endl;

  std::cout << "\n🎉 All examples completed successfully!" << std::endl;
  std::cout << "📚 Key differences:" << std::endl;
  std::cout << "   • Serializable: Fixed adapter, compile-time binding" << std::endl;
  std::cout << "   • MultiSerializable: Multiple adapters, runtime selection" << std::endl;
}

int main() {
  std::cout << "=== Lazy-CPP Serialization Examples ===" << std::endl;

  serializableExample();
  multiSerializableExample();

  return 0;
}
