#include <iostream>
#include <sstream>

#include "lazy/serialization/json.h"

// class MySealedClass {
//  public:
//   int myInt;
// };

// namespace lazy::serialization {
// template <>
// struct RapidJsonContext::CustomValueTypes<MySealedClass> {
//   constexpr static bool enabled = true;
//   static MySealedClass getValue(RapidJsonContext& context, RapidJsonContext::NodeType node) {
//     return MySealedClass();
//   }
//   static void setValue(RapidJsonContext& context, RapidJsonContext::NodeType node,
//                        MySealedClass value) {}
// };
// }  // namespace lazy::serialization

class MySubClass : public lazy::JsonSerializable<MySubClass> {
 public:
  SERIALIZABLE_FIELD(int, myInt);
};

class MyClass : public lazy::JsonSerializable<MyClass> {
 public:
  SERIALIZABLE_FIELD(int, myInt);
  SERIALIZABLE_FIELD(std::string, myString);
  SERIALIZABLE_FIELD(MySubClass, mySubClass);
};

int main() {
  MyClass myClass;
  myClass.myInt = 1;
  myClass.myString = "Hello, world!";
  myClass.mySubClass.myInt = 2;

  // Stream-based serialization (primary approach)
  std::stringstream ss;
  myClass.serialize(ss);
  std::cout << "Serialized JSON: " << ss.str() << std::endl;

  MyClass myClass2;
  ss.seekg(0);  // Reset stream position for reading
  myClass2.deserialize(ss);
  std::cout << "Deserialized myInt: " << myClass2.myInt << std::endl;
  std::cout << "Deserialized myString: " << myClass2.myString << std::endl;
}
