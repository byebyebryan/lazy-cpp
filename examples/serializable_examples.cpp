#include <iostream>
#include <sstream>

#include "lazy/serialization/text.h"

/*
  Derive from lazy::serialization::Serializable<T, ContextType> to make a class serializable

  ContextType will determine serialization format

  use SERIALIZABLE_FIELD to declare serializable fields

  Example:
  class TextClass : public lazy::serialization::Serializable<TextClass, TextContext>
  class JsonClass : public lazy::serialization::Serializable<JsonClass, JsonContext>
  class BinaryClass : public lazy::serialization::Serializable<BinaryClass, BinaryContext>

  Here we will use lazy::TextSerializable<T> = lazy::serialization::Serializable<T, TextContext>
*/

class MySubClass : public lazy::TextSerializable<MySubClass> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "MySubClass");
};

class MySealedClass {
 public:
  std::string name = "MySealedClass";
};

namespace lazy::serialization {

// make MySealedClass serializable non-intrusively
SERIALIZABLE_TYPE(TextContext, MySealedClass, name)

}  // namespace lazy::serialization

class MyClass : public lazy::TextSerializable<MyClass> {
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
