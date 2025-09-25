#include <iostream>

#include "lazy/serialization/json.h"

class MySealedClass {
 public:
  int myInt;
};

namespace lazy::serialization {
template <>
struct RapidJsonContext::CustomValueTypes<MySealedClass> {
  constexpr static bool enabled = true;
  static MySealedClass getValue(RapidJsonContext& context, RapidJsonContext::NodeType node) {
    return MySealedClass();
  }
  static void setValue(RapidJsonContext& context, RapidJsonContext::NodeType node,
                       MySealedClass value) {}
};
}  // namespace lazy::serialization

class MyClass : public lazy::JsonSerializable<MyClass> {
 public:
  SERIALIZABLE_FIELD(int, myInt);
  SERIALIZABLE_FIELD(std::string, myString);
  SERIALIZABLE_FIELD(MySealedClass, mySealedClass);
};

int main() {
  MyClass myClass;
  myClass.myInt = 1;
  myClass.myString = "Hello, world!";

  std::string json = myClass.serialize();
  std::cout << json << std::endl;

  MyClass myClass2;
  myClass2.deserialize(json);
  std::cout << myClass2.myInt << std::endl;
  std::cout << myClass2.myString << std::endl;
}
