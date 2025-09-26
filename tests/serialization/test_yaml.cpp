#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/yaml.h"

// ================================================================================
// YAML ADAPTER SPECIFIC TESTS
//
// This file tests YamlAdapter-specific functionality and basic integration
// with Serializable. Common serialization logic is tested in test_serializable.cpp.
// ================================================================================

// Simple test class for YAML serialization
class SimpleYamlClass : public lazy::YamlSerializable<SimpleYamlClass> {
 public:
  SERIALIZABLE_FIELD(int, id, 1);
  SERIALIZABLE_FIELD(std::string, name, "test");
  SERIALIZABLE_FIELD(double, score, 0.0);
  SERIALIZABLE_FIELD(bool, active, true);
};

// Class for testing integration
class YamlIntegrationClass : public lazy::YamlSerializable<YamlIntegrationClass> {
 public:
  SERIALIZABLE_FIELD(std::string, title, "yaml_integration");
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
  SERIALIZABLE_FIELD(SimpleYamlClass, nested);
};

// ================================================================================
// YamlAdapter Specific Tests
// ================================================================================

TEST(YamlAdapterTest, YamlFormatSyntax) {
  // Test that YAML format produces proper YAML syntax
  SimpleYamlClass obj;
  obj.id = 42;
  obj.name = "yaml_test";
  obj.score = 3.14;
  obj.active = false;

  std::ostringstream oss;
  obj.serialize(oss);
  std::string yamlOutput = oss.str();

  // Should use YAML key: value syntax
  EXPECT_TRUE(yamlOutput.find("id: 42") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("name: yaml_test") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("score: 3.14") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("active: false") != std::string::npos);

  // Should not have quotes around simple strings (YAML style)
  EXPECT_TRUE(yamlOutput.find("name: yaml_test") != std::string::npos);
  EXPECT_FALSE(yamlOutput.find("name: \"yaml_test\"") != std::string::npos);
}

TEST(YamlAdapterTest, NestedObjectStructure) {
  // Test that nested objects use proper YAML indentation
  YamlIntegrationClass obj;
  obj.title = "parent_yaml";
  obj.nested.id = 999;
  obj.nested.name = "nested_yaml";

  std::ostringstream oss;
  obj.serialize(oss);
  std::string yamlOutput = oss.str();

  // Should use YAML nested structure with indentation
  EXPECT_TRUE(yamlOutput.find("title: parent_yaml") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("nested:") != std::string::npos);
  // Nested fields should be indented
  EXPECT_TRUE(yamlOutput.find("  id: 999") != std::string::npos ||
              yamlOutput.find("\tid: 999") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("  name: nested_yaml") != std::string::npos ||
              yamlOutput.find("\tname: nested_yaml") != std::string::npos);
}

TEST(YamlAdapterTest, ArraySyntax) {
  // Test that arrays use proper YAML list syntax
  YamlIntegrationClass obj;
  obj.title = "array_test";
  obj.numbers = {1, 2, 3, 4, 5};

  std::ostringstream oss;
  obj.serialize(oss);
  std::string yamlOutput = oss.str();

  // Should use YAML list syntax
  EXPECT_TRUE(yamlOutput.find("numbers:") != std::string::npos);
  // Could be either inline [1, 2, 3, 4, 5] or block style with - elements
  bool hasInlineStyle = yamlOutput.find("[1,2,3,4,5]") != std::string::npos ||
                        yamlOutput.find("[1, 2, 3, 4, 5]") != std::string::npos;
  bool hasBlockStyle = yamlOutput.find("- 1") != std::string::npos;
  EXPECT_TRUE(hasInlineStyle || hasBlockStyle) << "Should use valid YAML list syntax";
}

TEST(YamlAdapterTest, ManualYamlParsing) {
  // Test that manually created YAML can be parsed
  std::string yamlData = R"(
id: 100
name: "manual_yaml"
score: 2.71
active: true
)";

  SimpleYamlClass obj;
  std::istringstream iss(yamlData);
  obj.deserialize(iss);

  EXPECT_EQ(obj.id, 100);
  EXPECT_EQ(obj.name, "manual_yaml");
  EXPECT_DOUBLE_EQ(obj.score, 2.71);
  EXPECT_TRUE(obj.active);
}

TEST(YamlAdapterTest, ComplexYamlParsing) {
  // Test complex nested YAML with arrays
  std::string yamlData = R"(
title: "complex_yaml"
nested:
  id: 777
  name: "deep_nested"
  score: 1.618
  active: false
numbers:
  - 10
  - 20
  - 30
)";

  YamlIntegrationClass obj;
  std::istringstream iss(yamlData);
  obj.deserialize(iss);

  EXPECT_EQ(obj.title, "complex_yaml");
  EXPECT_EQ(obj.nested.id, 777);
  EXPECT_EQ(obj.nested.name, "deep_nested");
  EXPECT_DOUBLE_EQ(obj.nested.score, 1.618);
  EXPECT_FALSE(obj.nested.active);
  EXPECT_EQ(obj.numbers.size(), 3u);
  EXPECT_EQ(obj.numbers[0], 10);
  EXPECT_EQ(obj.numbers[1], 20);
  EXPECT_EQ(obj.numbers[2], 30);
}

// ================================================================================
// Basic Integration Tests (Serializable + YamlAdapter)
// ================================================================================

TEST(YamlIntegrationTest, BasicSerializableIntegration) {
  // Simple sanity check that Serializable works with YamlAdapter
  YamlIntegrationClass original;
  original.title = "yaml_integration";
  original.numbers = {1, 2, 3};
  original.nested.id = 999;
  original.nested.name = "nested_yaml";

  std::ostringstream oss;
  original.serialize(oss);
  std::string yamlOutput = oss.str();

  // Should be valid YAML
  EXPECT_TRUE(yamlOutput.find("title: yaml_integration") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("numbers:") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("nested:") != std::string::npos);

  YamlIntegrationClass deserialized;
  std::istringstream iss(yamlOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(original.title, deserialized.title);
  EXPECT_EQ(original.numbers, deserialized.numbers);
  EXPECT_EQ(original.nested.id, deserialized.nested.id);
  EXPECT_EQ(original.nested.name, deserialized.nested.name);
}

TEST(YamlIntegrationTest, RoundTripConsistency) {
  // Test that YAML round-trip works correctly
  SimpleYamlClass original;
  original.id = 42;
  original.name = "roundtrip_yaml";
  original.score = 1.618;
  original.active = true;

  std::ostringstream oss;
  original.serialize(oss);

  SimpleYamlClass deserialized;
  std::istringstream iss(oss.str());
  deserialized.deserialize(iss);

  EXPECT_EQ(original.id, deserialized.id);
  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_DOUBLE_EQ(original.score, deserialized.score);
  EXPECT_EQ(original.active, deserialized.active);
}

TEST(YamlIntegrationTest, EmptyAndDefaultValues) {
  // Test empty vectors and default values work correctly
  YamlIntegrationClass obj;  // Uses defaults

  std::ostringstream oss;
  obj.serialize(oss);
  std::string yamlOutput = oss.str();

  // Should show default values and empty array
  EXPECT_TRUE(yamlOutput.find("title: yaml_integration") != std::string::npos);
  EXPECT_TRUE(yamlOutput.find("numbers: []") != std::string::npos ||
              yamlOutput.find("numbers:\n[]") != std::string::npos);

  YamlIntegrationClass deserialized;
  std::istringstream iss(yamlOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(obj.title, deserialized.title);
  EXPECT_TRUE(deserialized.numbers.empty());
  EXPECT_EQ(obj.nested.id, deserialized.nested.id);
  EXPECT_EQ(obj.nested.name, deserialized.nested.name);
}
