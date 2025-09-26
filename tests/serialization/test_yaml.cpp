#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/yaml.h"

// ================================================================================
// Test Classes and Setup
// ================================================================================

// Test classes using YamlAdapter
class YamlTestClass : public lazy::YamlSerializable<YamlTestClass> {
 public:
  SERIALIZABLE_FIELD(int, id, 1);
  SERIALIZABLE_FIELD(std::string, name, "test");
  SERIALIZABLE_FIELD(double, score);
  SERIALIZABLE_FIELD(bool, active, true);
};

class YamlNestedClass : public lazy::YamlSerializable<YamlNestedClass> {
 public:
  SERIALIZABLE_FIELD(std::string, title, "parent");
  SERIALIZABLE_FIELD(YamlTestClass, child);
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
};

// External class for testing SERIALIZABLE_TYPE with YamlAdapter
class YamlExternalClass {
 public:
  std::string label = "external";
  int count = 0;
};

namespace lazy::serialization {
SERIALIZABLE_TYPE(YamlAdapter, YamlExternalClass, label, count)
}

class YamlTestWithExternal : public lazy::YamlSerializable<YamlTestWithExternal> {
 public:
  SERIALIZABLE_FIELD(YamlExternalClass, external);
  SERIALIZABLE_FIELD(std::vector<std::string>, tags);
};

// Additional test classes for vector of nested and custom types
class YamlTestWithVectors : public lazy::YamlSerializable<YamlTestWithVectors> {
 public:
  SERIALIZABLE_FIELD(std::vector<YamlTestClass>, nestedObjects);
  SERIALIZABLE_FIELD(std::vector<YamlExternalClass>, customObjects);
  SERIALIZABLE_FIELD(std::string, name, "vector_test");
};

// ================================================================================
// Test Cases
// ================================================================================

TEST(YamlSerializationTest, BasicSerialization) {
  YamlTestClass obj;
  obj.id = 42;
  obj.name = "hello";
  obj.score = 3.14;
  obj.active = false;

  std::stringstream ss;
  obj.serialize(ss);

  std::string yamlStr = ss.str();
  EXPECT_TRUE(yamlStr.find("id: 42") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("name: hello") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("score: 3.14") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("active: false") != std::string::npos);
}

TEST(YamlSerializationTest, BasicDeserialization) {
  std::string yamlData = R"(
id: 100
name: "world"
score: 2.71
active: true
)";

  YamlTestClass obj;
  std::istringstream iss(yamlData);
  obj.deserialize(iss);

  EXPECT_EQ(obj.id, 100);
  EXPECT_EQ(obj.name, "world");
  EXPECT_DOUBLE_EQ(obj.score, 2.71);
  EXPECT_TRUE(obj.active);
}

TEST(YamlSerializationTest, DefaultValueSerialization) {
  YamlTestClass obj;  // Using default values

  std::stringstream ss;
  obj.serialize(ss);

  std::string yamlStr = ss.str();
  EXPECT_TRUE(yamlStr.find("id: 1") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("name: test") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("score: 0") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("active: true") != std::string::npos);
}

TEST(YamlSerializationTest, RoundTripSerialization) {
  YamlTestClass original;
  original.id = 999;
  original.name = "roundtrip";
  original.score = 1.618;
  original.active = false;

  std::stringstream ss;
  original.serialize(ss);

  YamlTestClass deserialized;
  std::istringstream iss(ss.str());
  deserialized.deserialize(iss);

  EXPECT_EQ(original.id, deserialized.id);
  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_DOUBLE_EQ(original.score, deserialized.score);
  EXPECT_EQ(original.active, deserialized.active);
}

TEST(YamlSerializationTest, NestedObjectSerialization) {
  YamlNestedClass obj;
  obj.title = "nested_test";
  obj.child.id = 42;
  obj.child.name = "nested_child";
  obj.child.score = 3.14;
  obj.child.active = true;
  obj.numbers = {1, 2, 3, 4, 5};

  std::stringstream ss;
  obj.serialize(ss);

  std::string yamlStr = ss.str();
  EXPECT_TRUE(yamlStr.find("title: nested_test") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("child:") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("id: 42") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("name: nested_child") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("numbers:") != std::string::npos);
}

TEST(YamlSerializationTest, NestedObjectDeserialization) {
  std::string yamlData = R"(
title: "parent_obj"
child:
  id: 123
  name: "child_obj"
  score: 9.87
  active: false
numbers:
  - 10
  - 20
  - 30
)";

  YamlNestedClass obj;
  std::istringstream iss(yamlData);
  obj.deserialize(iss);

  EXPECT_EQ(obj.title, "parent_obj");
  EXPECT_EQ(obj.child.id, 123);
  EXPECT_EQ(obj.child.name, "child_obj");
  EXPECT_DOUBLE_EQ(obj.child.score, 9.87);
  EXPECT_FALSE(obj.child.active);
  EXPECT_EQ(obj.numbers.size(), 3u);
  EXPECT_EQ(obj.numbers[0], 10);
  EXPECT_EQ(obj.numbers[1], 20);
  EXPECT_EQ(obj.numbers[2], 30);
}

TEST(YamlSerializationTest, NestedObjectRoundTrip) {
  YamlNestedClass original;
  original.title = "roundtrip_parent";
  original.child.id = 777;
  original.child.name = "roundtrip_child";
  original.child.score = 2.718;
  original.child.active = true;
  original.numbers = {7, 8, 9};

  std::stringstream ss;
  original.serialize(ss);

  YamlNestedClass deserialized;
  std::istringstream iss(ss.str());
  deserialized.deserialize(iss);

  EXPECT_EQ(original.title, deserialized.title);
  EXPECT_EQ(original.child.id, deserialized.child.id);
  EXPECT_EQ(original.child.name, deserialized.child.name);
  EXPECT_DOUBLE_EQ(original.child.score, deserialized.child.score);
  EXPECT_EQ(original.child.active, deserialized.child.active);
  EXPECT_EQ(original.numbers, deserialized.numbers);
}

TEST(YamlSerializationTest, YamlExternalClassSerialization) {
  YamlTestWithExternal obj;
  obj.external.label = "test_label";
  obj.external.count = 42;
  obj.tags = {"tag1", "tag2", "tag3"};

  std::stringstream ss;
  obj.serialize(ss);

  std::string yamlStr = ss.str();
  EXPECT_TRUE(yamlStr.find("external:") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("label: test_label") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("count: 42") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("tags:") != std::string::npos);
}

TEST(YamlSerializationTest, YamlExternalClassDeserialization) {
  std::string yamlData = R"(
external:
  label: "deserialized_label"
  count: 999
tags:
  - "yaml_tag1"
  - "yaml_tag2"
)";

  YamlTestWithExternal obj;
  std::istringstream iss(yamlData);
  obj.deserialize(iss);

  EXPECT_EQ(obj.external.label, "deserialized_label");
  EXPECT_EQ(obj.external.count, 999);
  EXPECT_EQ(obj.tags.size(), 2u);
  EXPECT_EQ(obj.tags[0], "yaml_tag1");
  EXPECT_EQ(obj.tags[1], "yaml_tag2");
}

TEST(YamlSerializationTest, YamlExternalClassRoundTrip) {
  YamlTestWithExternal original;
  original.external.label = "roundtrip_label";
  original.external.count = 123;
  original.tags = {"rt1", "rt2"};

  std::stringstream ss;
  original.serialize(ss);

  YamlTestWithExternal deserialized;
  std::istringstream iss(ss.str());
  deserialized.deserialize(iss);

  EXPECT_EQ(original.external.label, deserialized.external.label);
  EXPECT_EQ(original.external.count, deserialized.external.count);
  EXPECT_EQ(original.tags, deserialized.tags);
}

TEST(YamlSerializationTest, VectorOfObjectsSerialization) {
  YamlTestWithVectors obj;
  obj.name = "vector_test";

  // Add nested objects
  YamlTestClass nested1;
  nested1.id = 1;
  nested1.name = "nested1";
  nested1.score = 1.1;
  nested1.active = true;

  YamlTestClass nested2;
  nested2.id = 2;
  nested2.name = "nested2";
  nested2.score = 2.2;
  nested2.active = false;

  obj.nestedObjects = {nested1, nested2};

  // Add custom objects
  YamlExternalClass ext1;
  ext1.label = "ext1";
  ext1.count = 10;

  YamlExternalClass ext2;
  ext2.label = "ext2";
  ext2.count = 20;

  obj.customObjects = {ext1, ext2};

  std::stringstream ss;
  obj.serialize(ss);

  std::string yamlStr = ss.str();
  EXPECT_TRUE(yamlStr.find("name: vector_test") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("nestedObjects:") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("customObjects:") != std::string::npos);
}

TEST(YamlSerializationTest, VectorOfObjectsDeserialization) {
  std::string yamlData = R"(
name: "vector_deserialized"
nestedObjects:
  - id: 100
    name: "item1"
    score: 10.0
    active: true
  - id: 200
    name: "item2"
    score: 20.0
    active: false
customObjects:
  - label: "custom1"
    count: 5
  - label: "custom2"
    count: 15
)";

  YamlTestWithVectors obj;
  std::istringstream iss(yamlData);
  obj.deserialize(iss);

  EXPECT_EQ(obj.name, "vector_deserialized");
  EXPECT_EQ(obj.nestedObjects.size(), 2u);
  EXPECT_EQ(obj.nestedObjects[0].id, 100);
  EXPECT_EQ(obj.nestedObjects[0].name, "item1");
  EXPECT_DOUBLE_EQ(obj.nestedObjects[0].score, 10.0);
  EXPECT_TRUE(obj.nestedObjects[0].active);
  EXPECT_EQ(obj.nestedObjects[1].id, 200);
  EXPECT_EQ(obj.nestedObjects[1].name, "item2");
  EXPECT_DOUBLE_EQ(obj.nestedObjects[1].score, 20.0);
  EXPECT_FALSE(obj.nestedObjects[1].active);

  EXPECT_EQ(obj.customObjects.size(), 2u);
  EXPECT_EQ(obj.customObjects[0].label, "custom1");
  EXPECT_EQ(obj.customObjects[0].count, 5);
  EXPECT_EQ(obj.customObjects[1].label, "custom2");
  EXPECT_EQ(obj.customObjects[1].count, 15);
}

TEST(YamlSerializationTest, VectorOfObjectsRoundTrip) {
  YamlTestWithVectors original;
  original.name = "roundtrip_vectors";

  YamlTestClass nested;
  nested.id = 999;
  nested.name = "rt_nested";
  nested.score = 9.99;
  nested.active = true;
  original.nestedObjects = {nested};

  YamlExternalClass ext;
  ext.label = "rt_ext";
  ext.count = 777;
  original.customObjects = {ext};

  std::stringstream ss;
  original.serialize(ss);

  YamlTestWithVectors deserialized;
  std::istringstream iss(ss.str());
  deserialized.deserialize(iss);

  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_EQ(original.nestedObjects.size(), deserialized.nestedObjects.size());
  EXPECT_EQ(original.nestedObjects[0].id, deserialized.nestedObjects[0].id);
  EXPECT_EQ(original.nestedObjects[0].name, deserialized.nestedObjects[0].name);
  EXPECT_DOUBLE_EQ(original.nestedObjects[0].score, deserialized.nestedObjects[0].score);
  EXPECT_EQ(original.nestedObjects[0].active, deserialized.nestedObjects[0].active);

  EXPECT_EQ(original.customObjects.size(), deserialized.customObjects.size());
  EXPECT_EQ(original.customObjects[0].label, deserialized.customObjects[0].label);
  EXPECT_EQ(original.customObjects[0].count, deserialized.customObjects[0].count);
}

TEST(YamlSerializationTest, EmptyVectorSerialization) {
  YamlTestWithVectors obj;
  obj.name = "empty_vectors";
  // Leave vectors empty

  std::stringstream ss;
  obj.serialize(ss);

  std::string yamlStr = ss.str();
  EXPECT_TRUE(yamlStr.find("name: empty_vectors") != std::string::npos);
  EXPECT_TRUE(yamlStr.find("nestedObjects: []") != std::string::npos ||
              yamlStr.find("nestedObjects:\n[]") != std::string::npos ||
              yamlStr.find("nestedObjects:\n  []") != std::string::npos);
}

TEST(YamlSerializationTest, EmptyVectorDeserialization) {
  std::string yamlData = R"(
name: "empty_test"
nestedObjects: []
customObjects: []
)";

  YamlTestWithVectors obj;
  std::istringstream iss(yamlData);
  obj.deserialize(iss);

  EXPECT_EQ(obj.name, "empty_test");
  EXPECT_EQ(obj.nestedObjects.size(), 0u);
  EXPECT_EQ(obj.customObjects.size(), 0u);
}
