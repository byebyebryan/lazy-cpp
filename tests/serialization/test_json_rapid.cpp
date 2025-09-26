#include <gtest/gtest.h>
#include <rapidjson/document.h>

#include <sstream>

#include "lazy/serialization/json_rapid.h"

// ================================================================================
// RAPID JSON ADAPTER SPECIFIC TESTS
//
// This file tests RapidJsonAdapter-specific functionality and basic integration
// with Serializable. Common serialization logic is tested in test_serializable.cpp.
// Focus on RapidJSON library features and performance characteristics.
// ================================================================================

// Simple test class for RapidJSON serialization
class SimpleRapidJsonClass : public lazy::RapidJsonSerializable<SimpleRapidJsonClass> {
 public:
  SERIALIZABLE_FIELD(int, id, 1);
  SERIALIZABLE_FIELD(std::string, name, "test");
  SERIALIZABLE_FIELD(double, score, 0.0);
  SERIALIZABLE_FIELD(bool, active, true);
};

// Class for testing integration
class RapidJsonIntegrationClass : public lazy::RapidJsonSerializable<RapidJsonIntegrationClass> {
 public:
  SERIALIZABLE_FIELD(std::string, title, "rapid_json_integration");
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
  SERIALIZABLE_FIELD(SimpleRapidJsonClass, nested);
};

// ================================================================================
// Direct RapidJsonAdapter Tests
// ================================================================================

class RapidJsonAdapterTest : public ::testing::Test {
 protected:
  lazy::serialization::RapidJsonAdapter context;
};

TEST_F(RapidJsonAdapterTest, RapidJsonLibraryIntegration) {
  // Test direct RapidJSON library integration
  auto root = context.root();
  EXPECT_TRUE(context.isObject(root));

  // Add values and verify they work with RapidJSON library directly
  auto intChild = context.addChild(root, "rapidjson_test");
  context.setValue<int>(intChild, 42);

  auto stringChild = context.addChild(root, "string_field");
  context.setValue<std::string>(stringChild, "rapidjson_value");

  // Test that values are correctly stored
  EXPECT_EQ(context.getValue<int>(intChild), 42);
  EXPECT_EQ(context.getValue<std::string>(stringChild), "rapidjson_value");
}

TEST_F(RapidJsonAdapterTest, RapidJsonPerformanceFeatures) {
  // Test features specific to RapidJSON's performance characteristics
  auto root = context.root();
  auto arrayNode = context.addChild(root, "perf_array");

  context.setArray(arrayNode, 1000);  // Pre-allocate large array
  EXPECT_TRUE(context.isArray(arrayNode));

  // Add many elements efficiently (RapidJSON should handle this well)
  for (int i = 0; i < 100; ++i) {
    auto elem = context.addArrayElement(arrayNode);
    context.setValue<int>(elem, i * 10);
  }

  EXPECT_EQ(context.getArraySize(arrayNode), 100);

  // Verify random access (RapidJSON strength)
  auto elem50 = context.getArrayElement(arrayNode, 50);
  EXPECT_EQ(context.getValue<int>(elem50), 500);
}

TEST_F(RapidJsonAdapterTest, RapidJsonDocumentValidation) {
  // Test that output is valid RapidJSON Document
  std::ostringstream oss;
  lazy::serialization::RapidJsonAdapter streamContext(oss);

  auto root = streamContext.root();
  auto child = streamContext.addChild(root, "rapid_validation");
  streamContext.setValue<std::string>(child, "test_value");

  streamContext.finishSerialization();
  std::string jsonOutput = oss.str();

  // Verify with actual RapidJSON parsing
  rapidjson::Document doc;
  doc.Parse(jsonOutput.c_str());

  EXPECT_FALSE(doc.HasParseError());
  EXPECT_TRUE(doc.IsObject());
  EXPECT_TRUE(doc.HasMember("rapid_validation"));
  EXPECT_STREQ(doc["rapid_validation"].GetString(), "test_value");
}

TEST_F(RapidJsonAdapterTest, MemoryEfficiency) {
  // Test RapidJSON's memory handling (should not leak or crash with large data)
  auto root = context.root();

  // Create nested structure that would stress memory management
  for (int i = 0; i < 50; ++i) {
    auto childObj = context.addChild(root, "obj_" + std::to_string(i));
    context.setObject(childObj);

    auto arrayField = context.addChild(childObj, "array_field");
    context.setArray(arrayField, 10);

    for (int j = 0; j < 10; ++j) {
      auto elem = context.addArrayElement(arrayField);
      context.setValue<std::string>(elem, "value_" + std::to_string(i) + "_" + std::to_string(j));
    }
  }

  // Should not crash or leak memory
  EXPECT_TRUE(context.isObject(root));
}

// ================================================================================
// Basic Integration Tests (Serializable + RapidJsonAdapter)
// ================================================================================

TEST(RapidJsonIntegrationTest, BasicSerializableIntegration) {
  // Simple sanity check that Serializable works with RapidJsonAdapter
  RapidJsonIntegrationClass original;
  original.title = "rapid_json_integration";
  original.numbers = {1, 2, 3, 4, 5};
  original.nested.id = 999;
  original.nested.name = "nested_rapid";

  std::ostringstream oss;
  original.serialize(oss);
  std::string jsonOutput = oss.str();

  // Should be valid JSON
  rapidjson::Document doc;
  doc.Parse(jsonOutput.c_str());
  EXPECT_FALSE(doc.HasParseError());
  EXPECT_TRUE(doc.IsObject());

  RapidJsonIntegrationClass deserialized;
  std::istringstream iss(jsonOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(original.title, deserialized.title);
  EXPECT_EQ(original.numbers, deserialized.numbers);
  EXPECT_EQ(original.nested.id, deserialized.nested.id);
  EXPECT_EQ(original.nested.name, deserialized.nested.name);
}

TEST(RapidJsonIntegrationTest, RapidJsonSpecificValidation) {
  // Test RapidJSON-specific features
  SimpleRapidJsonClass obj;
  obj.id = 123;
  obj.name = "rapid_json_test";
  obj.score = 3.14159;
  obj.active = false;

  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonOutput = oss.str();

  // Use RapidJSON library directly to validate output
  rapidjson::Document doc;
  doc.Parse(jsonOutput.c_str());

  EXPECT_FALSE(doc.HasParseError());
  EXPECT_TRUE(doc.IsObject());
  EXPECT_TRUE(doc.HasMember("id"));
  EXPECT_TRUE(doc.HasMember("name"));
  EXPECT_TRUE(doc.HasMember("score"));
  EXPECT_TRUE(doc.HasMember("active"));

  EXPECT_EQ(doc["id"].GetInt(), 123);
  EXPECT_STREQ(doc["name"].GetString(), "rapid_json_test");
  EXPECT_DOUBLE_EQ(doc["score"].GetDouble(), 3.14159);
  EXPECT_FALSE(doc["active"].GetBool());
}

TEST(RapidJsonIntegrationTest, PartialDeserializationGraceful) {
  // Test RapidJSON handles partial JSON gracefully
  std::string partialJson = R"({"id": 42, "name": "partial_rapid"})";

  SimpleRapidJsonClass obj;
  std::istringstream iss(partialJson);
  obj.deserialize(iss);

  EXPECT_EQ(obj.id, 42);
  EXPECT_EQ(obj.name, "partial_rapid");
  // Missing fields should keep defaults
  EXPECT_DOUBLE_EQ(obj.score, 0.0);
  EXPECT_TRUE(obj.active);
}

TEST(RapidJsonIntegrationTest, EmptyAndDefaultValues) {
  // Test empty vectors and defaults work correctly with RapidJSON
  RapidJsonIntegrationClass obj;  // Uses defaults

  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonOutput = oss.str();

  RapidJsonIntegrationClass deserialized;
  std::istringstream iss(jsonOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(obj.title, deserialized.title);
  EXPECT_TRUE(deserialized.numbers.empty());
  EXPECT_EQ(obj.nested.id, deserialized.nested.id);
  EXPECT_EQ(obj.nested.name, deserialized.nested.name);
}
