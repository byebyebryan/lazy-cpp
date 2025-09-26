#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/text.h"

// ================================================================================
// TEXT ADAPTER SPECIFIC TESTS
//
// This file tests TextAdapter-specific functionality and basic integration
// with Serializable. Common serialization logic is tested in test_serializable.cpp.
// ================================================================================

// Simple test class for text serialization
class SimpleTextClass : public lazy::TextSerializable<SimpleTextClass> {
 public:
  SERIALIZABLE_FIELD(int, intField, 42);
  SERIALIZABLE_FIELD(std::string, stringField, "default");
  SERIALIZABLE_FIELD(double, doubleField, 3.14);
  SERIALIZABLE_FIELD(bool, boolField, true);
};

// Class for testing integration
class TextIntegrationClass : public lazy::TextSerializable<TextIntegrationClass> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "text_integration");
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
  SERIALIZABLE_FIELD(SimpleTextClass, nested);
};

// ================================================================================
// TextAdapter Specific Tests
// ================================================================================

class TextAdapterTest : public ::testing::Test {
 protected:
  lazy::serialization::TextAdapter context;
};

TEST_F(TextAdapterTest, HumanReadableFormat) {
  // Test that text format produces human-readable output
  SimpleTextClass obj;
  obj.intField = 123;
  obj.stringField = "test_value";
  obj.doubleField = 2.71;
  obj.boolField = false;

  std::ostringstream oss;
  obj.serialize(oss);
  std::string textOutput = oss.str();

  // Should be human-readable key=value format
  EXPECT_TRUE(textOutput.find("intField = 123") != std::string::npos);
  EXPECT_TRUE(textOutput.find("stringField = \"test_value\"") != std::string::npos);
  EXPECT_TRUE(textOutput.find("doubleField = 2.71") != std::string::npos);
  EXPECT_TRUE(textOutput.find("boolField = false") != std::string::npos);

  // Should not contain non-printable characters (unlike binary format)
  for (char c : textOutput) {
    EXPECT_TRUE(c == '\n' || c == '\r' || (c >= 32 && c <= 126))
        << "Text format should only contain printable characters and newlines";
  }
}

TEST_F(TextAdapterTest, DotNotationForNesting) {
  // Test that nested objects use dot notation
  TextIntegrationClass obj;
  obj.name = "parent";
  obj.nested.intField = 999;
  obj.nested.stringField = "child_value";

  std::ostringstream oss;
  obj.serialize(oss);
  std::string textOutput = oss.str();

  // Should use dot notation for nested fields
  EXPECT_TRUE(textOutput.find("name = \"parent\"") != std::string::npos);
  EXPECT_TRUE(textOutput.find("nested.intField = 999") != std::string::npos);
  EXPECT_TRUE(textOutput.find("nested.stringField = \"child_value\"") != std::string::npos);
}

TEST_F(TextAdapterTest, ArrayIndexNotation) {
  // Test that arrays use index notation
  TextIntegrationClass obj;
  obj.name = "array_test";
  obj.numbers = {10, 20, 30};

  std::ostringstream oss;
  obj.serialize(oss);
  std::string textOutput = oss.str();

  // Should use count and index notation for arrays
  EXPECT_TRUE(textOutput.find("numbers.count = 3") != std::string::npos);
  EXPECT_TRUE(textOutput.find("numbers.0 = 10") != std::string::npos);
  EXPECT_TRUE(textOutput.find("numbers.1 = 20") != std::string::npos);
  EXPECT_TRUE(textOutput.find("numbers.2 = 30") != std::string::npos);
}

TEST_F(TextAdapterTest, StringQuoting) {
  // Test that strings are properly quoted and escaped
  SimpleTextClass obj;
  obj.stringField = "String with \"quotes\" and\nnewlines";

  std::ostringstream oss;
  obj.serialize(oss);
  std::string textOutput = oss.str();

  // Should quote strings (escaping format may vary by implementation)
  EXPECT_TRUE(textOutput.find("stringField = \"") != std::string::npos);
  EXPECT_TRUE(textOutput.find("quotes") != std::string::npos);
  EXPECT_TRUE(textOutput.find("newlines") != std::string::npos);
}

TEST_F(TextAdapterTest, ManualFormatParsing) {
  // Test that manually created text format can be parsed
  std::stringstream input;
  input << "intField = 777\n";
  input << "stringField = \"manual_input\"\n";
  input << "doubleField = 9.99\n";
  input << "boolField = true\n";

  SimpleTextClass obj;
  input.seekg(0);
  obj.deserialize(input);

  EXPECT_EQ(obj.intField, 777);
  EXPECT_EQ(obj.stringField, "manual_input");
  EXPECT_DOUBLE_EQ(obj.doubleField, 9.99);
  EXPECT_EQ(obj.boolField, true);
}

// ================================================================================
// Basic Integration Tests (Serializable + TextAdapter)
// ================================================================================

TEST(TextIntegrationTest, BasicSerializableIntegration) {
  // Simple sanity check that Serializable works with TextAdapter
  TextIntegrationClass original;
  original.name = "text_integration";
  original.numbers = {1, 2, 3};
  original.nested.intField = 999;
  original.nested.stringField = "nested_text";

  std::ostringstream oss;
  original.serialize(oss);
  std::string textOutput = oss.str();

  // Should be human-readable
  EXPECT_TRUE(textOutput.find("name = \"text_integration\"") != std::string::npos);
  EXPECT_TRUE(textOutput.find("numbers.count = 3") != std::string::npos);
  EXPECT_TRUE(textOutput.find("nested.intField = 999") != std::string::npos);

  TextIntegrationClass deserialized;
  std::istringstream iss(textOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_EQ(original.numbers, deserialized.numbers);
  EXPECT_EQ(original.nested.intField, deserialized.nested.intField);
  EXPECT_EQ(original.nested.stringField, deserialized.nested.stringField);
}

TEST(TextIntegrationTest, RoundTripConsistency) {
  // Test that serialization -> deserialization -> serialization produces same output
  SimpleTextClass original;
  original.intField = 42;
  original.stringField = "consistency_test";
  original.doubleField = 1.618;
  original.boolField = true;

  // First serialization
  std::ostringstream oss1;
  original.serialize(oss1);
  std::string firstOutput = oss1.str();

  // Deserialize
  SimpleTextClass intermediate;
  std::istringstream iss(firstOutput);
  intermediate.deserialize(iss);

  // Second serialization
  std::ostringstream oss2;
  intermediate.serialize(oss2);
  std::string secondOutput = oss2.str();

  // Should be identical (or at least functionally equivalent)
  EXPECT_EQ(firstOutput, secondOutput);
}

TEST(TextIntegrationTest, EmptyAndDefaultValues) {
  // Test empty vectors and default values work correctly
  TextIntegrationClass obj;  // Uses defaults

  std::ostringstream oss;
  obj.serialize(oss);
  std::string textOutput = oss.str();

  // Should show default values and empty array
  EXPECT_TRUE(textOutput.find("name = \"text_integration\"") != std::string::npos);
  EXPECT_TRUE(textOutput.find("numbers.count = 0") != std::string::npos);
  EXPECT_TRUE(textOutput.find("nested.intField = 42") != std::string::npos);  // default

  TextIntegrationClass deserialized;
  std::istringstream iss(textOutput);
  deserialized.deserialize(iss);

  EXPECT_EQ(obj.name, deserialized.name);
  EXPECT_TRUE(deserialized.numbers.empty());
  EXPECT_EQ(obj.nested.intField, deserialized.nested.intField);
  EXPECT_EQ(obj.nested.stringField, deserialized.nested.stringField);
}
