#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <sstream>

#include "lazy/serialization/json_rapid.h"

// ================================================================================
// Test Classes and Setup
// ================================================================================

// Test classes using RapidJsonAdapter
class RapidJsonTestClass : public lazy::RapidJsonSerializable<RapidJsonTestClass> {
 public:
  SERIALIZABLE_FIELD(int, id, 1);
  SERIALIZABLE_FIELD(std::string, name, "test");
  SERIALIZABLE_FIELD(double, score);
  SERIALIZABLE_FIELD(bool, active, true);
};

class RapidJsonNestedClass : public lazy::RapidJsonSerializable<RapidJsonNestedClass> {
 public:
  SERIALIZABLE_FIELD(std::string, title, "parent");
  SERIALIZABLE_FIELD(RapidJsonTestClass, child);
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
};

// External class for testing SERIALIZABLE_TYPE with RapidJsonAdapter
class JsonExternalClass {
 public:
  std::string label = "external";
  int count = 0;
};

namespace lazy::serialization {
SERIALIZABLE_TYPE(RapidJsonAdapter, JsonExternalClass, label, count)
}

class RapidJsonTestWithExternal : public lazy::RapidJsonSerializable<RapidJsonTestWithExternal> {
 public:
  SERIALIZABLE_FIELD(JsonExternalClass, external);
  SERIALIZABLE_FIELD(std::vector<std::string>, tags);
};

// Additional test classes for vector of nested and custom types
class RapidJsonTestWithVectors : public lazy::RapidJsonSerializable<RapidJsonTestWithVectors> {
 public:
  SERIALIZABLE_FIELD(std::vector<RapidJsonTestClass>, nestedObjects);
  SERIALIZABLE_FIELD(std::vector<JsonExternalClass>, customObjects);
  SERIALIZABLE_FIELD(std::string, name, "vector_test");
};

// ================================================================================
// RapidJsonAdapter Unit Tests
// ================================================================================

class RapidJsonAdapterTest : public ::testing::Test {
 protected:
  lazy::serialization::RapidJsonAdapter context;
};

TEST_F(RapidJsonAdapterTest, BasicNodeOperations) {
  auto root = context.root();
  EXPECT_TRUE(context.isObject(root));

  // Test adding child nodes
  auto child = context.addChild(root, "test_field");
  EXPECT_NE(child, nullptr);

  // Test setting values
  context.setValue<int>(child, 42);
  EXPECT_EQ(context.getValue<int>(child), 42);

  context.setValue<std::string>(child, "hello");
  EXPECT_EQ(context.getValue<std::string>(child), "hello");
}

TEST_F(RapidJsonAdapterTest, ArrayOperations) {
  auto root = context.root();
  auto arrayNode = context.addChild(root, "array_field");

  context.setArray(arrayNode, 3);
  EXPECT_TRUE(context.isArray(arrayNode));
  EXPECT_EQ(context.getArraySize(arrayNode), 0);  // Initially empty

  // Add elements
  auto elem1 = context.addArrayElement(arrayNode);
  context.setValue<int>(elem1, 10);

  auto elem2 = context.addArrayElement(arrayNode);
  context.setValue<int>(elem2, 20);

  EXPECT_EQ(context.getArraySize(arrayNode), 2);

  // Retrieve elements
  auto retrieved1 = context.getArrayElement(arrayNode, 0);
  auto retrieved2 = context.getArrayElement(arrayNode, 1);

  EXPECT_EQ(context.getValue<int>(retrieved1), 10);
  EXPECT_EQ(context.getValue<int>(retrieved2), 20);
}

TEST_F(RapidJsonAdapterTest, StreamOperations) {
  std::ostringstream oss;
  lazy::serialization::RapidJsonAdapter streamContext(oss);
  auto root = streamContext.root();
  auto child = streamContext.addChild(root, "stream_test");
  streamContext.setValue<std::string>(child, "stream_value");

  // finishSerialization() writes to the stream passed in constructor
  streamContext.finishSerialization();
  std::string json = oss.str();

  EXPECT_FALSE(json.empty());
  EXPECT_NE(json.find("stream_test"), std::string::npos);
  EXPECT_NE(json.find("stream_value"), std::string::npos);

  // Test parsing from stream
  std::istringstream iss(json);
  lazy::serialization::RapidJsonAdapter context2(iss);

  auto root2 = context2.root();
  auto child2 = context2.getChild(root2, "stream_test");
  EXPECT_NE(child2, nullptr);
  EXPECT_EQ(context2.getValue<std::string>(child2), "stream_value");
}

TEST_F(RapidJsonAdapterTest, EmptyKeyHandling) {
  auto root = context.root();

  // Empty key should return the node itself
  EXPECT_EQ(context.addChild(root, ""), root);
  EXPECT_EQ(context.getChild(root, ""), root);
}

// ================================================================================
// JSON Serialization Integration Tests
// ================================================================================

class RapidJsonSerializableTest : public ::testing::Test {
 protected:
  template <typename T>
  std::string serialize(const T& obj) {
    std::ostringstream oss;
    obj.serialize(oss);
    return oss.str();
  }

  template <typename T>
  T deserialize(const std::string& json) {
    std::istringstream iss(json);
    T obj;
    obj.deserialize(iss);
    return obj;
  }
};

// --- Basic Serialization Tests ---

TEST_F(RapidJsonSerializableTest, BasicSerializationDeserialization) {
  RapidJsonTestClass original;
  original.id = 42;
  original.name = "hello";
  original.score = 95.5;
  original.active = false;

  std::string json = serialize(original);
  EXPECT_FALSE(json.empty());

  RapidJsonTestClass deserialized = deserialize<RapidJsonTestClass>(json);

  EXPECT_EQ(deserialized.id, 42);
  EXPECT_EQ(deserialized.name, "hello");
  EXPECT_DOUBLE_EQ(deserialized.score, 95.5);
  EXPECT_EQ(deserialized.active, false);
}

TEST_F(RapidJsonSerializableTest, DefaultValues) {
  RapidJsonTestClass obj;  // Uses default values

  std::string json = serialize(obj);
  RapidJsonTestClass deserialized = deserialize<RapidJsonTestClass>(json);

  EXPECT_EQ(deserialized.id, 1);
  EXPECT_EQ(deserialized.name, "test");
  EXPECT_DOUBLE_EQ(deserialized.score, 0.0);
  EXPECT_EQ(deserialized.active, true);
}

// --- Nested Object Tests ---

TEST_F(RapidJsonSerializableTest, NestedObjectSerialization) {
  RapidJsonNestedClass original;
  original.title = "parent_obj";
  original.child.id = 100;
  original.child.name = "child_obj";
  original.child.score = 88.8;
  original.numbers = {1, 2, 3, 4, 5};

  std::string json = serialize(original);
  RapidJsonNestedClass deserialized = deserialize<RapidJsonNestedClass>(json);

  EXPECT_EQ(deserialized.title, "parent_obj");
  EXPECT_EQ(deserialized.child.id, 100);
  EXPECT_EQ(deserialized.child.name, "child_obj");
  EXPECT_DOUBLE_EQ(deserialized.child.score, 88.8);
  EXPECT_EQ(deserialized.numbers, std::vector<int>({1, 2, 3, 4, 5}));
}

// --- Simple Vector Tests ---

TEST_F(RapidJsonSerializableTest, VectorSerialization) {
  RapidJsonNestedClass obj;
  obj.numbers = {10, 20, 30};

  std::string json = serialize(obj);
  RapidJsonNestedClass deserialized = deserialize<RapidJsonNestedClass>(json);

  EXPECT_EQ(deserialized.numbers.size(), 3);
  EXPECT_EQ(deserialized.numbers[0], 10);
  EXPECT_EQ(deserialized.numbers[1], 20);
  EXPECT_EQ(deserialized.numbers[2], 30);
}

// --- External Class Tests ---

TEST_F(RapidJsonSerializableTest, ExternalClassSerialization) {
  RapidJsonTestWithExternal original;
  original.external.label = "custom_label";
  original.external.count = 999;
  original.tags = {"tag1", "tag2", "tag3"};

  std::string json = serialize(original);
  RapidJsonTestWithExternal deserialized = deserialize<RapidJsonTestWithExternal>(json);

  EXPECT_EQ(deserialized.external.label, "custom_label");
  EXPECT_EQ(deserialized.external.count, 999);
  EXPECT_EQ(deserialized.tags.size(), 3);
  EXPECT_EQ(deserialized.tags[0], "tag1");
  EXPECT_EQ(deserialized.tags[1], "tag2");
  EXPECT_EQ(deserialized.tags[2], "tag3");
}

// --- Edge Cases and Validation Tests ---

TEST_F(RapidJsonSerializableTest, EmptyVectorSerialization) {
  RapidJsonTestWithExternal obj;
  obj.tags.clear();  // Empty vector

  std::string json = serialize(obj);
  RapidJsonTestWithExternal deserialized = deserialize<RapidJsonTestWithExternal>(json);

  EXPECT_TRUE(deserialized.tags.empty());
}

TEST_F(RapidJsonSerializableTest, JsonValidityCheck) {
  RapidJsonTestClass obj;
  obj.id = 123;
  obj.name = "json_test";

  std::string json = serialize(obj);

  // Parse JSON to verify it's valid
  rapidjson::Document doc;
  doc.Parse(json.c_str());

  EXPECT_FALSE(doc.HasParseError());
  EXPECT_TRUE(doc.IsObject());
  EXPECT_TRUE(doc.HasMember("id"));
  EXPECT_TRUE(doc.HasMember("name"));
  EXPECT_EQ(doc["id"].GetInt(), 123);
  EXPECT_EQ(std::string(doc["name"].GetString()), "json_test");
}

TEST_F(RapidJsonSerializableTest, PartialDeserialization) {
  // Test deserialization when some fields are missing from JSON
  std::string partialJson = R"({"id": 42, "name": "partial"})";

  RapidJsonTestClass obj = deserialize<RapidJsonTestClass>(partialJson);

  EXPECT_EQ(obj.id, 42);
  EXPECT_EQ(obj.name, "partial");
  EXPECT_DOUBLE_EQ(obj.score, 0.0);  // Should remain default/zero
  EXPECT_EQ(obj.active, true);       // Should remain default
}

// --- Complex Vector Tests ---

TEST_F(RapidJsonSerializableTest, VectorOfNestedObjects) {
  RapidJsonTestWithVectors original;

  // Create nested objects
  RapidJsonTestClass obj1;
  obj1.id = 1;
  obj1.name = "first";
  obj1.score = 10.5;
  obj1.active = true;

  RapidJsonTestClass obj2;
  obj2.id = 2;
  obj2.name = "second";
  obj2.score = 20.5;
  obj2.active = false;

  original.nestedObjects = {obj1, obj2};
  original.name = "nested_vector_test";

  std::string json = serialize(original);
  RapidJsonTestWithVectors deserialized = deserialize<RapidJsonTestWithVectors>(json);

  EXPECT_EQ(deserialized.name, "nested_vector_test");
  EXPECT_EQ(deserialized.nestedObjects.size(), 2);

  EXPECT_EQ(deserialized.nestedObjects[0].id, 1);
  EXPECT_EQ(deserialized.nestedObjects[0].name, "first");
  EXPECT_DOUBLE_EQ(deserialized.nestedObjects[0].score, 10.5);
  EXPECT_EQ(deserialized.nestedObjects[0].active, true);

  EXPECT_EQ(deserialized.nestedObjects[1].id, 2);
  EXPECT_EQ(deserialized.nestedObjects[1].name, "second");
  EXPECT_DOUBLE_EQ(deserialized.nestedObjects[1].score, 20.5);
  EXPECT_EQ(deserialized.nestedObjects[1].active, false);
}

TEST_F(RapidJsonSerializableTest, VectorOfCustomObjects) {
  RapidJsonTestWithVectors original;

  // Create custom objects
  JsonExternalClass custom1;
  custom1.label = "custom_one";
  custom1.count = 100;

  JsonExternalClass custom2;
  custom2.label = "custom_two";
  custom2.count = 200;

  original.customObjects = {custom1, custom2};
  original.name = "custom_vector_test";

  std::string json = serialize(original);
  RapidJsonTestWithVectors deserialized = deserialize<RapidJsonTestWithVectors>(json);

  EXPECT_EQ(deserialized.name, "custom_vector_test");
  EXPECT_EQ(deserialized.customObjects.size(), 2);

  EXPECT_EQ(deserialized.customObjects[0].label, "custom_one");
  EXPECT_EQ(deserialized.customObjects[0].count, 100);

  EXPECT_EQ(deserialized.customObjects[1].label, "custom_two");
  EXPECT_EQ(deserialized.customObjects[1].count, 200);
}

TEST_F(RapidJsonSerializableTest, EmptyVectorsOfComplexTypes) {
  RapidJsonTestWithVectors obj;
  // Keep vectors empty
  obj.nestedObjects.clear();
  obj.customObjects.clear();
  obj.name = "empty_vectors_test";

  std::string json = serialize(obj);
  RapidJsonTestWithVectors deserialized = deserialize<RapidJsonTestWithVectors>(json);

  EXPECT_EQ(deserialized.name, "empty_vectors_test");
  EXPECT_TRUE(deserialized.nestedObjects.empty());
  EXPECT_TRUE(deserialized.customObjects.empty());
}

TEST_F(RapidJsonSerializableTest, MixedComplexVectors) {
  RapidJsonTestWithVectors original;

  // Single nested object
  RapidJsonTestClass nested;
  nested.id = 42;
  nested.name = "mixed_test";
  nested.score = 3.14;
  nested.active = true;
  original.nestedObjects = {nested};

  // Multiple custom objects
  JsonExternalClass custom1;
  custom1.label = "alpha";
  custom1.count = 1;

  JsonExternalClass custom2;
  custom2.label = "beta";
  custom2.count = 2;

  JsonExternalClass custom3;
  custom3.label = "gamma";
  custom3.count = 3;

  original.customObjects = {custom1, custom2, custom3};
  original.name = "mixed_complex_test";

  std::string json = serialize(original);
  RapidJsonTestWithVectors deserialized = deserialize<RapidJsonTestWithVectors>(json);

  // Verify nested objects vector
  EXPECT_EQ(deserialized.nestedObjects.size(), 1);
  EXPECT_EQ(deserialized.nestedObjects[0].id, 42);
  EXPECT_EQ(deserialized.nestedObjects[0].name, "mixed_test");
  EXPECT_DOUBLE_EQ(deserialized.nestedObjects[0].score, 3.14);
  EXPECT_EQ(deserialized.nestedObjects[0].active, true);

  // Verify custom objects vector
  EXPECT_EQ(deserialized.customObjects.size(), 3);
  EXPECT_EQ(deserialized.customObjects[0].label, "alpha");
  EXPECT_EQ(deserialized.customObjects[0].count, 1);
  EXPECT_EQ(deserialized.customObjects[1].label, "beta");
  EXPECT_EQ(deserialized.customObjects[1].count, 2);
  EXPECT_EQ(deserialized.customObjects[2].label, "gamma");
  EXPECT_EQ(deserialized.customObjects[2].count, 3);

  EXPECT_EQ(deserialized.name, "mixed_complex_test");
}
