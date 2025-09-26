#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/json_lazy.h"

// ================================================================================
// Test Classes and Setup
// ================================================================================

// Test classes using LazyJsonAdapter
class LazyJsonTestClass : public lazy::LazyJsonSerializable<LazyJsonTestClass> {
 public:
  SERIALIZABLE_FIELD(int, id, 1);
  SERIALIZABLE_FIELD(std::string, name, "test");
  SERIALIZABLE_FIELD(double, score);
  SERIALIZABLE_FIELD(bool, active, true);
};

class LazyJsonNestedClass : public lazy::LazyJsonSerializable<LazyJsonNestedClass> {
 public:
  SERIALIZABLE_FIELD(std::string, title, "parent");
  SERIALIZABLE_FIELD(LazyJsonTestClass, child);
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
};

// External class for testing SERIALIZABLE_TYPE with LazyJsonAdapter
class LazyJsonExternalClass {
 public:
  std::string label = "external";
  int count = 0;
};

namespace lazy::serialization {
SERIALIZABLE_TYPE(LazyJsonAdapter, LazyJsonExternalClass, label, count)
}

class LazyJsonTestWithExternal : public lazy::LazyJsonSerializable<LazyJsonTestWithExternal> {
 public:
  SERIALIZABLE_FIELD(LazyJsonExternalClass, external);
  SERIALIZABLE_FIELD(std::vector<std::string>, tags);
};

// Additional test classes for vector of nested and custom types
class LazyJsonTestWithVectors : public lazy::LazyJsonSerializable<LazyJsonTestWithVectors> {
 public:
  SERIALIZABLE_FIELD(std::vector<LazyJsonTestClass>, nestedObjects);
  SERIALIZABLE_FIELD(std::vector<LazyJsonExternalClass>, customObjects);
  SERIALIZABLE_FIELD(std::string, name, "vector_test");
};

// ================================================================================
// Basic Serialization/Deserialization Tests
// ================================================================================

TEST(LazyJsonSerializationTest, BasicTypes) {
  LazyJsonTestClass original;
  original.id = 42;
  original.name = "hello";
  original.score = 95.5;
  original.active = false;

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);
  std::string jsonStr = oss.str();

  // Should contain expected values
  EXPECT_TRUE(jsonStr.find("42") != std::string::npos);
  EXPECT_TRUE(jsonStr.find("hello") != std::string::npos);
  EXPECT_TRUE(jsonStr.find("95.5") != std::string::npos);
  EXPECT_TRUE(jsonStr.find("false") != std::string::npos);

  // Deserialize
  LazyJsonTestClass deserialized;
  std::istringstream iss(jsonStr);
  deserialized.deserialize(iss);

  // Verify values
  EXPECT_EQ(original.id, deserialized.id);
  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_DOUBLE_EQ(original.score, deserialized.score);
  EXPECT_EQ(original.active, deserialized.active);
}

TEST(LazyJsonSerializationTest, DefaultValues) {
  LazyJsonTestClass obj;  // Use defaults

  // Serialize
  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonStr = oss.str();

  // Deserialize
  LazyJsonTestClass deserialized;
  std::istringstream iss(jsonStr);
  deserialized.deserialize(iss);

  // Should match defaults
  EXPECT_EQ(obj.id, deserialized.id);          // 1
  EXPECT_EQ(obj.name, deserialized.name);      // "test"
  EXPECT_EQ(obj.score, deserialized.score);    // 0.0
  EXPECT_EQ(obj.active, deserialized.active);  // true
}

TEST(LazyJsonSerializationTest, NestedObjects) {
  LazyJsonNestedClass original;
  original.title = "parent_obj";
  original.child.id = 100;
  original.child.name = "nested";
  original.child.score = 88.8;
  original.child.active = false;
  original.numbers = {1, 2, 3, 4, 5};

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);
  std::string jsonStr = oss.str();

  // Deserialize
  LazyJsonNestedClass deserialized;
  std::istringstream iss(jsonStr);
  deserialized.deserialize(iss);

  // Verify parent values
  EXPECT_EQ(original.title, deserialized.title);

  // Verify child values
  EXPECT_EQ(original.child.id, deserialized.child.id);
  EXPECT_EQ(original.child.name, deserialized.child.name);
  EXPECT_DOUBLE_EQ(original.child.score, deserialized.child.score);
  EXPECT_EQ(original.child.active, deserialized.child.active);

  // Verify array values
  EXPECT_EQ(original.numbers, deserialized.numbers);
}

TEST(LazyJsonSerializationTest, ExternalClass) {
  LazyJsonTestWithExternal original;
  original.external.label = "custom";
  original.external.count = 42;
  original.tags = {"tag1", "tag2", "tag3"};

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);
  std::string jsonStr = oss.str();

  // Deserialize
  LazyJsonTestWithExternal deserialized;
  std::istringstream iss(jsonStr);
  deserialized.deserialize(iss);

  // Verify external object
  EXPECT_EQ(original.external.label, deserialized.external.label);
  EXPECT_EQ(original.external.count, deserialized.external.count);

  // Verify tags array
  EXPECT_EQ(original.tags, deserialized.tags);
}

TEST(LazyJsonSerializationTest, VectorOfObjects) {
  LazyJsonTestWithVectors original;
  original.name = "vector_container";

  // Add nested objects
  LazyJsonTestClass obj1;
  obj1.id = 10;
  obj1.name = "first";
  obj1.score = 1.1;

  LazyJsonTestClass obj2;
  obj2.id = 20;
  obj2.name = "second";
  obj2.score = 2.2;

  original.nestedObjects = {obj1, obj2};

  // Add custom objects
  LazyJsonExternalClass ext1;
  ext1.label = "ext1";
  ext1.count = 100;

  LazyJsonExternalClass ext2;
  ext2.label = "ext2";
  ext2.count = 200;

  original.customObjects = {ext1, ext2};

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);
  std::string jsonStr = oss.str();

  // Deserialize
  LazyJsonTestWithVectors deserialized;
  std::istringstream iss(jsonStr);
  deserialized.deserialize(iss);

  // Verify container name
  EXPECT_EQ(original.name, deserialized.name);

  // Verify nested objects
  ASSERT_EQ(original.nestedObjects.size(), deserialized.nestedObjects.size());
  for (size_t i = 0; i < original.nestedObjects.size(); ++i) {
    EXPECT_EQ(original.nestedObjects[i].id, deserialized.nestedObjects[i].id);
    EXPECT_EQ(original.nestedObjects[i].name, deserialized.nestedObjects[i].name);
    EXPECT_DOUBLE_EQ(original.nestedObjects[i].score, deserialized.nestedObjects[i].score);
  }

  // Verify custom objects
  ASSERT_EQ(original.customObjects.size(), deserialized.customObjects.size());
  for (size_t i = 0; i < original.customObjects.size(); ++i) {
    EXPECT_EQ(original.customObjects[i].label, deserialized.customObjects[i].label);
    EXPECT_EQ(original.customObjects[i].count, deserialized.customObjects[i].count);
  }
}

// ================================================================================
// Edge Cases and Error Handling Tests
// ================================================================================

TEST(LazyJsonSerializationTest, EmptyString) {
  LazyJsonTestClass obj;

  // Try deserializing empty string - should use defaults gracefully
  std::istringstream iss("");
  obj.deserialize(iss);

  // Should have default values
  EXPECT_EQ(1, obj.id);
  EXPECT_EQ("test", obj.name);
  EXPECT_DOUBLE_EQ(0.0, obj.score);
  EXPECT_TRUE(obj.active);
}

TEST(LazyJsonSerializationTest, InvalidJson) {
  LazyJsonTestClass obj;

  // Try deserializing invalid JSON - should handle gracefully
  std::istringstream iss("{invalid json}");
  obj.deserialize(iss);

  // Should have default values (graceful handling)
  EXPECT_EQ(1, obj.id);
  EXPECT_EQ("test", obj.name);
  EXPECT_DOUBLE_EQ(0.0, obj.score);
  EXPECT_TRUE(obj.active);
}

TEST(LazyJsonSerializationTest, MissingFields) {
  LazyJsonTestClass obj;

  // JSON with only some fields
  std::string partialJson = R"({"id": 99, "name": "partial"})";
  std::istringstream iss(partialJson);
  obj.deserialize(iss);

  // Should have provided values and defaults for missing ones
  EXPECT_EQ(99, obj.id);
  EXPECT_EQ("partial", obj.name);
  EXPECT_DOUBLE_EQ(0.0, obj.score);  // default
  EXPECT_TRUE(obj.active);           // default
}

TEST(LazyJsonSerializationTest, TypeMismatch) {
  LazyJsonTestClass obj;

  // JSON with wrong types - should handle gracefully with defaults
  std::string mismatchJson =
      R"({"id": "not_a_number", "name": 123, "score": "not_a_double", "active": "not_a_bool"})";
  std::istringstream iss(mismatchJson);
  obj.deserialize(iss);

  // Should use default values for type mismatches
  EXPECT_EQ(0, obj.id);     // default for failed number parse
  EXPECT_EQ("", obj.name);  // default for string
  EXPECT_DOUBLE_EQ(0.0, obj.score);
  EXPECT_FALSE(obj.active);  // default for failed bool parse
}

TEST(LazyJsonSerializationTest, EmptyArrays) {
  LazyJsonNestedClass obj;
  obj.title = "empty_test";
  obj.numbers = {};  // empty array

  // Serialize
  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonStr = oss.str();

  // Should contain empty array
  EXPECT_TRUE(jsonStr.find("[]") != std::string::npos);

  // Deserialize
  LazyJsonNestedClass deserialized;
  std::istringstream iss(jsonStr);
  deserialized.deserialize(iss);

  // Should have empty array
  EXPECT_EQ(obj.title, deserialized.title);
  EXPECT_TRUE(deserialized.numbers.empty());
}

// ================================================================================
// JSON Format Validation Tests
// ================================================================================

TEST(LazyJsonSerializationTest, ValidJsonOutput) {
  LazyJsonTestClass obj;
  obj.id = 42;
  obj.name = "test with \"quotes\" and \\backslashes\\";
  obj.score = 3.14159;
  obj.active = true;

  // Serialize
  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonStr = oss.str();

  // Basic JSON structure checks
  EXPECT_TRUE(jsonStr.front() == '{');
  EXPECT_TRUE(jsonStr.back() == '}');

  // Should properly escape strings
  EXPECT_TRUE(jsonStr.find("\\\"") != std::string::npos);  // escaped quotes
  EXPECT_TRUE(jsonStr.find("\\\\") != std::string::npos);  // escaped backslashes

  // Should be parseable back
  LazyJsonTestClass deserialized;
  std::istringstream iss(jsonStr);
  deserialized.deserialize(iss);

  EXPECT_EQ(obj.id, deserialized.id);
  EXPECT_EQ(obj.name, deserialized.name);
  EXPECT_DOUBLE_EQ(obj.score, deserialized.score);
  EXPECT_EQ(obj.active, deserialized.active);
}

// ================================================================================
// Direct LazyJson Tests
// ================================================================================

class LazyJsonTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

TEST_F(LazyJsonTest, BasicJsonParsingAndSerialization) {
  // Test deserialization
  std::string jsonStr = R"({"name":"test","value":42,"active":true})";
  std::istringstream iss(jsonStr);

  lazy::serialization::LazyJson json(iss);
  auto root = json.root();

  // Test getting values
  auto nameNode = json.getChild(root, "name");
  ASSERT_NE(nameNode, nullptr);
  EXPECT_EQ(json.getValue<std::string>(nameNode), "test");

  auto valueNode = json.getChild(root, "value");
  ASSERT_NE(valueNode, nullptr);
  EXPECT_EQ(json.getValue<int>(valueNode), 42);

  auto activeNode = json.getChild(root, "active");
  ASSERT_NE(activeNode, nullptr);
  EXPECT_EQ(json.getValue<bool>(activeNode), true);
}

TEST_F(LazyJsonTest, SerializationMode) {
  // Test serialization mode
  lazy::serialization::LazyJson json;
  EXPECT_TRUE(json.isSerializing());

  auto root = json.root();
  auto child = json.addChild(root, "test");
  ASSERT_NE(child, nullptr);

  json.setValue(child, std::string("hello"));

  std::string result = json.toJsonString();
  EXPECT_NE(result.find("\"test\":\"hello\""), std::string::npos);
}

TEST_F(LazyJsonTest, DeserializationMode) {
  std::string jsonStr = R"({"key":"value"})";
  std::istringstream iss(jsonStr);

  lazy::serialization::LazyJson json(iss);
  EXPECT_FALSE(json.isSerializing());

  // Should not be able to add children in deserialization mode
  auto root = json.root();
  auto child = json.addChild(root, "newkey");
  EXPECT_EQ(child, nullptr);
}

TEST_F(LazyJsonTest, ArrayOperations) {
  // Test serialization of arrays
  lazy::serialization::LazyJson json;
  auto root = json.root();

  json.setArray(root, 3);
  EXPECT_TRUE(json.isArray(root));

  auto elem1 = json.addArrayElement(root);
  ASSERT_NE(elem1, nullptr);
  json.setValue(elem1, 10);

  auto elem2 = json.addArrayElement(root);
  ASSERT_NE(elem2, nullptr);
  json.setValue(elem2, 20);

  std::string result = json.toJsonString();
  EXPECT_NE(result.find("[10,20]"), std::string::npos);
}

TEST_F(LazyJsonTest, ArrayDeserialization) {
  std::string jsonStr = R"([1,2,3,4,5])";
  std::istringstream iss(jsonStr);

  lazy::serialization::LazyJson json(iss);
  auto root = json.root();

  EXPECT_TRUE(json.isArray(root));
  EXPECT_EQ(json.getArraySize(root), 5);

  auto elem0 = json.getArrayElement(root, 0);
  ASSERT_NE(elem0, nullptr);
  EXPECT_EQ(json.getValue<int>(elem0), 1);

  auto elem4 = json.getArrayElement(root, 4);
  ASSERT_NE(elem4, nullptr);
  EXPECT_EQ(json.getValue<int>(elem4), 5);

  // Out of bounds should return nullptr
  auto elemOut = json.getArrayElement(root, 10);
  EXPECT_EQ(elemOut, nullptr);
}

TEST_F(LazyJsonTest, TypeMismatchHandling) {
  std::string jsonStr = R"({"str":"hello","num":42,"bool":true})";
  std::istringstream iss(jsonStr);

  lazy::serialization::LazyJson json(iss);
  auto root = json.root();

  auto strNode = json.getChild(root, "str");
  ASSERT_NE(strNode, nullptr);

  // Correct type
  EXPECT_EQ(json.getValue<std::string>(strNode), "hello");

  // Type mismatch should return default
  EXPECT_EQ(json.getValue<int>(strNode), 0);
  EXPECT_EQ(json.getValue<bool>(strNode), false);
}

TEST_F(LazyJsonTest, EmptyKeyHandling) {
  lazy::serialization::LazyJson json;
  auto root = json.root();

  // Empty key should return the same node
  auto sameNode = json.addChild(root, "");
  EXPECT_EQ(sameNode, root);
}

TEST_F(LazyJsonTest, ComplexNestedStructure) {
  std::string jsonStr = R"({
    "user": {
      "name": "Alice",
      "scores": [95, 87, 92]
    },
    "metadata": {
      "created": "2024-01-01",
      "tags": ["test", "example"]
    }
  })";

  std::istringstream iss(jsonStr);
  lazy::serialization::LazyJson json(iss);
  auto root = json.root();

  // Navigate to nested object
  auto userNode = json.getChild(root, "user");
  ASSERT_NE(userNode, nullptr);

  auto nameNode = json.getChild(userNode, "name");
  ASSERT_NE(nameNode, nullptr);
  EXPECT_EQ(json.getValue<std::string>(nameNode), "Alice");

  auto scoresNode = json.getChild(userNode, "scores");
  ASSERT_NE(scoresNode, nullptr);
  EXPECT_TRUE(json.isArray(scoresNode));
  EXPECT_EQ(json.getArraySize(scoresNode), 3);

  auto score0 = json.getArrayElement(scoresNode, 0);
  ASSERT_NE(score0, nullptr);
  EXPECT_EQ(json.getValue<int>(score0), 95);
}

TEST_F(LazyJsonTest, JsonStringEscaping) {
  lazy::serialization::LazyJson json;
  auto root = json.root();

  auto child = json.addChild(root, "test");
  ASSERT_NE(child, nullptr);

  // Test string with special characters
  std::string specialStr = "Hello \"world\"\nWith\ttabs";
  json.setValue(child, specialStr);

  std::string result = json.toJsonString();

  // Should contain escaped characters
  EXPECT_NE(result.find("\\\""), std::string::npos);
  EXPECT_NE(result.find("\\n"), std::string::npos);
  EXPECT_NE(result.find("\\t"), std::string::npos);
}
