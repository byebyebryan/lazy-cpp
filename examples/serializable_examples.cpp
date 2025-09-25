#include <iostream>
#include <sstream>

#include "lazy/serialization/json.h"

class MySubClass : public lazy::JsonSerializable<MySubClass> {
  public:
   SERIALIZABLE_FIELD(std::string, name, "MySubClass");
 };

class MySealedClass {
 public:
  std::string name = "MySealedClass";
};

namespace lazy::serialization {
template <>
struct RapidJsonContext::CustomValueTypes<MySealedClass> {
  constexpr static bool enabled = true;
  static MySealedClass getValue(RapidJsonContext& context, RapidJsonContext::NodeType node) {
    MySealedClass result;
    if (!context.isObject(node)) {
      return result;
    }
    if (auto nameNode = context.getChild(node, "name"); nameNode) {
      result.name = context.getValue<std::string>(nameNode);
    }
    return result;
  }
  static void setValue(RapidJsonContext& context, RapidJsonContext::NodeType node,
                       MySealedClass value) {
    context.setObject(node);
    auto nameNode = context.addChild(node, "name");
    context.setValue<std::string>(nameNode, value.name);
  }
};
}  // namespace lazy::serialization

class MyClass : public lazy::JsonSerializable<MyClass> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "MyClass");
  SERIALIZABLE_FIELD(int, value);
  SERIALIZABLE_FIELD(MySubClass, subClass);
  SERIALIZABLE_FIELD(MySealedClass, sealedClass);
};

int main() {
  MyClass myClass;

  std::stringstream ss;
  myClass.serialize(ss);
  std::cout << "Serialized Default: " << ss.str() << std::endl;

  myClass.name = "MyClass2";
  myClass.value = 2;
  myClass.subClass.name = "MySubClass2";
  myClass.sealedClass.name = "MySealedClass2";

  std::stringstream ss2;
  myClass.serialize(ss2);
  std::cout << "Serialized Modified: " << ss2.str() << std::endl;

  MyClass myClass2;
  ss2.seekg(0);
  myClass2.deserialize(ss2);
  std::cout << "Deserialized Modified: \n"
            << "name: " << myClass2.name << "\n"
            << "value: " << myClass2.value << "\n"
            << "subClass.name: " << myClass2.subClass.name << "\n"
            << "sealedClass.name: " << myClass2.sealedClass.name << std::endl;
}
