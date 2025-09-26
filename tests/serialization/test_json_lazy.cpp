#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/json_lazy.h"

// ================================================================================
// LAZY JSON ADAPTER AND PARSER SPECIFIC TESTS
//
// This file tests LazyJsonAdapter-specific functionality, direct LazyJson parser
// usage, and basic integration with Serializable. Common serialization logic
// is tested in test_serializable.cpp.
// ================================================================================

// Simple test class for LazyJson serialization
class SimpleLazyJsonClass : public lazy::LazyJsonSerializable<SimpleLazyJsonClass> {
 public:
  SERIALIZABLE_FIELD(int, id, 1);
  SERIALIZABLE_FIELD(std::string, name, "test");
  SERIALIZABLE_FIELD(double, score, 0.0);
  SERIALIZABLE_FIELD(bool, active, true);
};

// Class for testing integration
class LazyJsonIntegrationClass : public lazy::LazyJsonSerializable<LazyJsonIntegrationClass> {
 public:
  SERIALIZABLE_FIELD(std::string, title, "json_integration");
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
  SERIALIZABLE_FIELD(SimpleLazyJsonClass, nested);
};

// ================================================================================
// LazyJsonAdapter Specific Tests
// ================================================================================

TEST(LazyJsonAdapterTest, JsonFormatValidation) {
  // Test that LazyJson produces valid JSON format
  SimpleLazyJsonClass obj;
  obj.id = 42;
  obj.name = "json_test";
  obj.score = 3.14;
  obj.active = false;

  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonOutput = oss.str();

  // Should be valid JSON format
  EXPECT_TRUE(jsonOutput.front() == '{');
  EXPECT_TRUE(jsonOutput.back() == '}');
  EXPECT_TRUE(jsonOutput.find("\"id\":42") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"name\":\"json_test\"") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"score\":3.14") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"active\":false") != std::string::npos);

  // Should be compact (no unnecessary whitespace)
  EXPECT_EQ(jsonOutput.find("  "), std::string::npos);  // No double spaces
  EXPECT_EQ(jsonOutput.find("\n"), std::string::npos);  // No newlines
}

TEST(LazyJsonAdapterTest, StringEscaping) {
  // Test that strings are properly escaped in JSON
  SimpleLazyJsonClass obj;
  obj.name = "String with \"quotes\" and\nnewlines";

  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonOutput = oss.str();

  // Should escape quotes and newlines
  EXPECT_TRUE(jsonOutput.find("\\\"") != std::string::npos);  // Escaped quotes
  EXPECT_TRUE(jsonOutput.find("\\n") != std::string::npos);   // Escaped newlines

  // Should not have unescaped quotes/newlines
  size_t nameStart = jsonOutput.find("\"name\":\"") + 8;  // After opening quote
  size_t nameEnd = jsonOutput.find("\",", nameStart);
  if (nameEnd == std::string::npos) nameEnd = jsonOutput.find("\"}", nameStart);
  std::string nameValue = jsonOutput.substr(nameStart, nameEnd - nameStart);

  // Count unescaped quotes in the value (should be 0, all should be escaped as \")
  size_t unescapedQuotes = 0;
  for (size_t i = 0; i < nameValue.length(); ++i) {
    if (nameValue[i] == '"' && (i == 0 || nameValue[i - 1] != '\\')) {
      unescapedQuotes++;
    }
  }
  EXPECT_EQ(unescapedQuotes, 0) << "All quotes should be escaped in JSON strings";
}

TEST(LazyJsonAdapterTest, ArraySyntax) {
  // Test that arrays use proper JSON array syntax
  LazyJsonIntegrationClass obj;
  obj.title = "array_test";
  obj.numbers = {1, 2, 3, 4, 5};

  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonOutput = oss.str();

  // Should use JSON array syntax
  EXPECT_TRUE(jsonOutput.find("\"numbers\":[1,2,3,4,5]") != std::string::npos);
}

TEST(LazyJsonAdapterTest, NestedObjectSyntax) {
  // Test nested objects use proper JSON object syntax
  LazyJsonIntegrationClass obj;
  obj.title = "nested_test";
  obj.nested.id = 999;
  obj.nested.name = "nested_object";

  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonOutput = oss.str();

  // Should use nested JSON object syntax
  EXPECT_TRUE(jsonOutput.find("\"nested\":{") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"id\":999") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"name\":\"nested_object\"") != std::string::npos);
}

// ================================================================================
// Direct LazyJson Parser Tests
// ================================================================================

class LazyJsonParserTest : public ::testing::Test {
 protected:
  void SetUp() override {}
};

TEST_F(LazyJsonParserTest, BasicJsonParsingAndSerialization) {
  // Test direct usage of LazyJson parser for basic JSON operations
  std::string jsonStr = R"({"name":"test","value":42,"active":true})";
  std::istringstream iss(jsonStr);

  lazy::serialization::LazyJson json(iss);
  auto root = json.root();

  // Test getting values with direct parser access
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

TEST_F(LazyJsonParserTest, SerializationMode) {
  // Test LazyJson in serialization mode
  lazy::serialization::LazyJson json;
  EXPECT_TRUE(json.isSerializing());

  auto root = json.root();
  auto child = json.addChild(root, "test");
  ASSERT_NE(child, nullptr);

  json.setValue(child, std::string("hello"));

  std::string result = json.toJsonString();
  EXPECT_TRUE(result.find("\"test\":\"hello\"") != std::string::npos);
}

TEST_F(LazyJsonParserTest, DeserializationMode) {
  // Test LazyJson in deserialization mode
  std::string jsonStr = R"({"key":"value"})";
  std::istringstream iss(jsonStr);

  lazy::serialization::LazyJson json(iss);
  EXPECT_FALSE(json.isSerializing());

  // Should not be able to add children in deserialization mode
  auto root = json.root();
  auto child = json.addChild(root, "newkey");
  EXPECT_EQ(child, nullptr);
}

TEST_F(LazyJsonParserTest, ArrayOperations) {
  // Test LazyJson array operations
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
  EXPECT_TRUE(result.find("[10,20]") != std::string::npos);
}

TEST_F(LazyJsonParserTest, ArrayDeserialization) {
  // Test LazyJson array deserialization
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

TEST_F(LazyJsonParserTest, TypeMismatchHandling) {
  // Test LazyJson type mismatch handling
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

TEST_F(LazyJsonParserTest, ComplexNestedStructure) {
  // Test LazyJson with complex nested JSON
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

TEST_F(LazyJsonParserTest, JsonStringEscaping) {
  // Test LazyJson string escaping
  lazy::serialization::LazyJson json;
  auto root = json.root();

  auto child = json.addChild(root, "test");
  ASSERT_NE(child, nullptr);

  // Test string with special characters
  std::string specialStr = "Hello \"world\"\nWith\ttabs";
  json.setValue(child, specialStr);

  std::string result = json.toJsonString();

  // Should contain escaped characters
  EXPECT_TRUE(result.find("\\\"") != std::string::npos);
  EXPECT_TRUE(result.find("\\n") != std::string::npos);
  EXPECT_TRUE(result.find("\\t") != std::string::npos);
}

TEST_F(LazyJsonParserTest, EmptyKeyHandling) {
  // Test LazyJson empty key handling
  lazy::serialization::LazyJson json;
  auto root = json.root();

  // Empty key should return the same node
  auto sameNode = json.addChild(root, "");
  EXPECT_EQ(sameNode, root);
}

// ================================================================================
// Basic Integration Tests (Serializable + LazyJsonAdapter)
// ================================================================================

TEST(LazyJsonIntegrationTest, BasicSerializableIntegration) {
  // Simple sanity check that Serializable works with LazyJsonAdapter
  LazyJsonIntegrationClass original;
  original.title = "json_integration";
  original.numbers = {1, 2, 3, 4, 5};
  original.nested.id = 999;
  original.nested.name = "nested_json";

  std::ostringstream oss;
  original.serialize(oss);
  std::string jsonOutput = oss.str();

  // Should be valid JSON
  EXPECT_TRUE(jsonOutput.front() == '{');
  EXPECT_TRUE(jsonOutput.back() == '}');
  EXPECT_TRUE(jsonOutput.find("\"title\":\"json_integration\"") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"numbers\":[1,2,3,4,5]") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"nested\":{") != std::string::npos);

  LazyJsonIntegrationClass deserialized;
  std::istringstream iss(jsonOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(original.title, deserialized.title);
  EXPECT_EQ(original.numbers, deserialized.numbers);
  EXPECT_EQ(original.nested.id, deserialized.nested.id);
  EXPECT_EQ(original.nested.name, deserialized.nested.name);
}

TEST(LazyJsonIntegrationTest, RoundTripConsistency) {
  // Test that JSON round-trip works correctly
  SimpleLazyJsonClass original;
  original.id = 42;
  original.name = "roundtrip_json";
  original.score = 1.618;
  original.active = true;

  std::ostringstream oss;
  original.serialize(oss);

  SimpleLazyJsonClass deserialized;
  std::istringstream iss(oss.str());
  deserialized.deserialize(iss);

  EXPECT_EQ(original.id, deserialized.id);
  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_DOUBLE_EQ(original.score, deserialized.score);
  EXPECT_EQ(original.active, deserialized.active);
}

TEST(LazyJsonIntegrationTest, ErrorHandling) {
  // Test that LazyJson handles malformed JSON gracefully
  SimpleLazyJsonClass obj;

  // Test with empty string
  std::istringstream iss1("");
  EXPECT_NO_THROW(obj.deserialize(iss1));

  // Test with invalid JSON
  std::istringstream iss2("{invalid json}");
  EXPECT_NO_THROW(obj.deserialize(iss2));

  // Test with partial JSON (missing fields should use defaults)
  std::istringstream iss3(R"({"id":99,"name":"partial"})");
  obj.deserialize(iss3);
  EXPECT_EQ(obj.id, 99);
  EXPECT_EQ(obj.name, "partial");
  // Missing fields should have default values
}

TEST(LazyJsonIntegrationTest, EmptyAndDefaultValues) {
  // Test empty vectors and default values work correctly
  LazyJsonIntegrationClass obj;  // Uses defaults

  std::ostringstream oss;
  obj.serialize(oss);
  std::string jsonOutput = oss.str();

  // Should show default values and empty array
  EXPECT_TRUE(jsonOutput.find("\"title\":\"json_integration\"") != std::string::npos);
  EXPECT_TRUE(jsonOutput.find("\"numbers\":[]") != std::string::npos);

  LazyJsonIntegrationClass deserialized;
  std::istringstream iss(jsonOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(obj.title, deserialized.title);
  EXPECT_TRUE(deserialized.numbers.empty());
  EXPECT_EQ(obj.nested.id, deserialized.nested.id);
  EXPECT_EQ(obj.nested.name, deserialized.nested.name);
}
