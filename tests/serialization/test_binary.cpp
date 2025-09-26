#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/binary.h"

// ================================================================================
// BINARY ADAPTER SPECIFIC TESTS
//
// This file tests BinaryAdapter-specific functionality and basic integration
// with Serializable. Common serialization logic is tested in test_serializable.cpp.
// ================================================================================

// Simple test class for basic binary serialization
class SimpleBinaryClass : public lazy::BinarySerializable<SimpleBinaryClass> {
 public:
  SERIALIZABLE_FIELD(int32_t, intField, 42);
  SERIALIZABLE_FIELD(std::string, stringField, "default");
  SERIALIZABLE_FIELD(double, doubleField, 3.14);
  SERIALIZABLE_FIELD(bool, boolField, true);
};

// Class for testing integration
class TestIntegrationClass : public lazy::BinarySerializable<TestIntegrationClass> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "integration_test");
  SERIALIZABLE_FIELD(std::vector<int>, numbers);
  SERIALIZABLE_FIELD(SimpleBinaryClass, nested);
};

// ================================================================================
// BinaryAdapter Specific Tests
// ================================================================================

TEST(BinaryAdapterTest, BinaryFormatCompactness) {
  SimpleBinaryClass obj;
  obj.intField = 42;
  obj.stringField = "test";
  obj.doubleField = 3.14;
  obj.boolField = true;

  std::ostringstream oss;
  obj.serialize(oss);
  std::string binaryOutput = oss.str();

  // Binary format should be more compact than text formats
  EXPECT_LT(binaryOutput.size(), 100u);  // Should be compact
  EXPECT_GT(binaryOutput.size(), 10u);   // But not unreasonably small

  // Binary data should not be human-readable (contains non-printable chars)
  bool hasNonPrintable = false;
  for (char c : binaryOutput) {
    if (static_cast<unsigned char>(c) < 32 || static_cast<unsigned char>(c) > 126) {
      hasNonPrintable = true;
      break;
    }
  }
  EXPECT_TRUE(hasNonPrintable) << "Binary format should contain non-printable characters";
}

TEST(BinaryAdapterTest, PrimitiveTypesRoundTrip) {
  SimpleBinaryClass original;
  original.intField = 12345;
  original.stringField = "binary test";
  original.doubleField = 98.765;
  original.boolField = false;

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  SimpleBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify exact precision is maintained in binary format
  EXPECT_EQ(original.intField, deserialized.intField);
  EXPECT_EQ(original.stringField, deserialized.stringField);
  EXPECT_DOUBLE_EQ(original.doubleField, deserialized.doubleField);
  EXPECT_EQ(original.boolField, deserialized.boolField);
}

TEST(BinaryAdapterTest, UnicodeStringHandling) {
  // Test that binary format handles Unicode correctly
  SimpleBinaryClass original;
  original.stringField = "Hello 世界 🌍 Тест";

  std::ostringstream oss;
  original.serialize(oss);

  std::istringstream iss(oss.str());
  SimpleBinaryClass deserialized;
  deserialized.deserialize(iss);

  EXPECT_EQ(original.stringField, deserialized.stringField);
}

TEST(BinaryAdapterTest, FloatingPointPrecision) {
  // Test that binary format maintains floating point precision
  SimpleBinaryClass original;
  original.doubleField = 3.141592653589793238462643;  // High precision double

  std::ostringstream oss;
  original.serialize(oss);

  std::istringstream iss(oss.str());
  SimpleBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Binary format should maintain exact precision
  EXPECT_DOUBLE_EQ(original.doubleField, deserialized.doubleField);
}

TEST(BinaryAdapterTest, EndiannessConsistency) {
  // Test that binary format is consistent across serialize/deserialize cycles
  SimpleBinaryClass original;
  original.intField = 0x12345678;  // Distinct byte pattern for endianness testing

  std::ostringstream oss;
  original.serialize(oss);
  std::string serialized = oss.str();

  // Multiple deserializations should be consistent
  for (int i = 0; i < 3; ++i) {
    std::istringstream iss(serialized);
    SimpleBinaryClass deserialized;
    deserialized.deserialize(iss);
    EXPECT_EQ(original.intField, deserialized.intField);
  }
}

// ================================================================================
// Basic Integration Tests (Serializable + BinaryAdapter)
// ================================================================================

TEST(BinaryIntegrationTest, BasicSerializableIntegration) {
  // Simple sanity check that Serializable works with BinaryAdapter
  TestIntegrationClass original;
  original.name = "binary_integration";
  original.numbers = {1, 2, 3, 4, 5};
  original.nested.intField = 999;
  original.nested.stringField = "nested_binary";

  std::ostringstream oss;
  original.serialize(oss);
  std::string binaryData = oss.str();

  // Should be binary (not human readable)
  EXPECT_GT(binaryData.size(), 0u);

  TestIntegrationClass deserialized;
  std::istringstream iss(binaryData);
  deserialized.deserialize(iss);

  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_EQ(original.numbers, deserialized.numbers);
  EXPECT_EQ(original.nested.intField, deserialized.nested.intField);
  EXPECT_EQ(original.nested.stringField, deserialized.nested.stringField);
}

TEST(BinaryIntegrationTest, EmptyAndDefaultValues) {
  // Test empty vectors and default values work correctly
  TestIntegrationClass
      obj;  // Uses defaults: name="integration_test", numbers=empty, nested=defaults

  std::ostringstream oss;
  obj.serialize(oss);

  TestIntegrationClass deserialized;
  std::istringstream iss(oss.str());
  deserialized.deserialize(iss);

  EXPECT_EQ(obj.name, deserialized.name);
  EXPECT_TRUE(deserialized.numbers.empty());
  EXPECT_EQ(obj.nested.intField, deserialized.nested.intField);
  EXPECT_EQ(obj.nested.stringField, deserialized.nested.stringField);
}
