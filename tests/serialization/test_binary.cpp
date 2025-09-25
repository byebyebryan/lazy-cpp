#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/binary.h"

// ================================================================================
// Test Classes and Setup
// ================================================================================

class SimpleBinaryClass : public lazy::BinarySerializable<SimpleBinaryClass> {
 public:
  SERIALIZABLE_FIELD(int32_t, intField, 42);
  SERIALIZABLE_FIELD(std::string, stringField, "default");
  SERIALIZABLE_FIELD(double, doubleField, 3.14);
  SERIALIZABLE_FIELD(bool, boolField, true);
  SERIALIZABLE_FIELD(float, floatField, 2.5f);
  SERIALIZABLE_FIELD(int16_t, shortField, 100);
  SERIALIZABLE_FIELD(uint32_t, unsignedField, 99u);
};

class NestedBinaryClass : public lazy::BinarySerializable<NestedBinaryClass> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "nested");
  SERIALIZABLE_FIELD(SimpleBinaryClass, nestedObject);
};

class ArrayBinaryClass : public lazy::BinarySerializable<ArrayBinaryClass> {
 public:
  SERIALIZABLE_FIELD(std::vector<int32_t>, intVector);
  SERIALIZABLE_FIELD(std::vector<std::string>, stringVector);
  SERIALIZABLE_FIELD(std::vector<SimpleBinaryClass>, objectVector);
};

// External class for SERIALIZABLE_TYPE testing
class ExternalClass {
 public:
  int32_t value = 123;
  std::string description = "external";
  bool flag = false;
};

namespace lazy::serialization {
SERIALIZABLE_TYPE(BinaryAdapter, ExternalClass, value, description, flag)
}

class BinaryClassWithExternal : public lazy::BinarySerializable<BinaryClassWithExternal> {
 public:
  SERIALIZABLE_FIELD(ExternalClass, externalField);
  SERIALIZABLE_FIELD(std::string, name, "with_external");
};

// Complex nested structure for comprehensive testing
class ComplexBinaryClass : public lazy::BinarySerializable<ComplexBinaryClass> {
 public:
  SERIALIZABLE_FIELD(std::vector<NestedBinaryClass>, nestedVector);
  SERIALIZABLE_FIELD(std::vector<std::vector<int32_t>>, nestedArrays);
  SERIALIZABLE_FIELD(ArrayBinaryClass, arrayObject);
};

// ================================================================================
// Basic Binary Serialization Tests
// ================================================================================

TEST(BinarySerializationTest, SimplePrimitiveTypes) {
  SimpleBinaryClass original;
  original.intField = 12345;
  original.stringField = "test string";
  original.doubleField = 98.765;
  original.boolField = false;
  original.floatField = 1.23f;
  original.shortField = -456;
  original.unsignedField = 4294967295u;  // Max uint32_t

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);
  std::string serialized = oss.str();
  EXPECT_GT(serialized.size(), 0u);

  // Deserialize
  std::istringstream iss(serialized);
  SimpleBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify all fields
  EXPECT_EQ(original.intField, deserialized.intField);
  EXPECT_EQ(original.stringField, deserialized.stringField);
  EXPECT_DOUBLE_EQ(original.doubleField, deserialized.doubleField);
  EXPECT_EQ(original.boolField, deserialized.boolField);
  EXPECT_FLOAT_EQ(original.floatField, deserialized.floatField);
  EXPECT_EQ(original.shortField, deserialized.shortField);
  EXPECT_EQ(original.unsignedField, deserialized.unsignedField);
}

TEST(BinarySerializationTest, EmptyStringsAndZeroValues) {
  SimpleBinaryClass original;
  original.intField = 0;
  original.stringField = "";
  original.doubleField = 0.0;
  original.boolField = false;
  original.floatField = 0.0f;
  original.shortField = 0;
  original.unsignedField = 0u;

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  SimpleBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify all fields
  EXPECT_EQ(original.intField, deserialized.intField);
  EXPECT_EQ(original.stringField, deserialized.stringField);
  EXPECT_DOUBLE_EQ(original.doubleField, deserialized.doubleField);
  EXPECT_EQ(original.boolField, deserialized.boolField);
  EXPECT_FLOAT_EQ(original.floatField, deserialized.floatField);
  EXPECT_EQ(original.shortField, deserialized.shortField);
  EXPECT_EQ(original.unsignedField, deserialized.unsignedField);
}

TEST(BinarySerializationTest, UnicodeStrings) {
  SimpleBinaryClass original;
  original.stringField = "Hello 世界 🌍 Тест ñoñó";

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  SimpleBinaryClass deserialized;
  deserialized.deserialize(iss);

  EXPECT_EQ(original.stringField, deserialized.stringField);
}

// ================================================================================
// Nested Object Tests
// ================================================================================

TEST(BinarySerializationTest, NestedObjects) {
  NestedBinaryClass original;
  original.name = "parent_object";
  original.nestedObject.intField = 999;
  original.nestedObject.stringField = "nested_value";
  original.nestedObject.doubleField = 123.456;
  original.nestedObject.boolField = true;

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  NestedBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify parent fields
  EXPECT_EQ(original.name, deserialized.name);

  // Verify nested object fields
  EXPECT_EQ(original.nestedObject.intField, deserialized.nestedObject.intField);
  EXPECT_EQ(original.nestedObject.stringField, deserialized.nestedObject.stringField);
  EXPECT_DOUBLE_EQ(original.nestedObject.doubleField, deserialized.nestedObject.doubleField);
  EXPECT_EQ(original.nestedObject.boolField, deserialized.nestedObject.boolField);
}

// ================================================================================
// Array Tests
// ================================================================================

TEST(BinarySerializationTest, PrimitiveArrays) {
  ArrayBinaryClass original;
  original.intVector = {1, 2, 3, 4, 5};
  original.stringVector = {"first", "second", "third"};

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  ArrayBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify arrays
  EXPECT_EQ(original.intVector, deserialized.intVector);
  EXPECT_EQ(original.stringVector, deserialized.stringVector);
}

TEST(BinarySerializationTest, ObjectArrays) {
  ArrayBinaryClass original;

  // Add objects to the array
  for (int i = 0; i < 3; ++i) {
    SimpleBinaryClass obj;
    obj.intField = 100 + i;
    obj.stringField = "object_" + std::to_string(i);
    obj.doubleField = i * 1.5;
    obj.boolField = (i % 2 == 0);
    original.objectVector.push_back(obj);
  }

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  ArrayBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify object array
  ASSERT_EQ(original.objectVector.size(), deserialized.objectVector.size());
  for (size_t i = 0; i < original.objectVector.size(); ++i) {
    EXPECT_EQ(original.objectVector[i].intField, deserialized.objectVector[i].intField);
    EXPECT_EQ(original.objectVector[i].stringField, deserialized.objectVector[i].stringField);
    EXPECT_DOUBLE_EQ(original.objectVector[i].doubleField,
                     deserialized.objectVector[i].doubleField);
    EXPECT_EQ(original.objectVector[i].boolField, deserialized.objectVector[i].boolField);
  }
}

TEST(BinarySerializationTest, EmptyArrays) {
  ArrayBinaryClass original;
  // Leave all arrays empty

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  ArrayBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify empty arrays
  EXPECT_TRUE(deserialized.intVector.empty());
  EXPECT_TRUE(deserialized.stringVector.empty());
  EXPECT_TRUE(deserialized.objectVector.empty());
}

// ================================================================================
// External Type Tests (SERIALIZABLE_TYPE)
// ================================================================================

TEST(BinarySerializationTest, ExternalTypes) {
  BinaryClassWithExternal original;
  original.name = "test_with_external";
  original.externalField.value = 789;
  original.externalField.description = "external_test";
  original.externalField.flag = true;

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  BinaryClassWithExternal deserialized;
  deserialized.deserialize(iss);

  // Verify fields
  EXPECT_EQ(original.name, deserialized.name);
  EXPECT_EQ(original.externalField.value, deserialized.externalField.value);
  EXPECT_EQ(original.externalField.description, deserialized.externalField.description);
  EXPECT_EQ(original.externalField.flag, deserialized.externalField.flag);
}

// ================================================================================
// Complex Structure Tests
// ================================================================================

TEST(BinarySerializationTest, ComplexNestedStructures) {
  ComplexBinaryClass original;

  // Create nested objects
  for (int i = 0; i < 2; ++i) {
    NestedBinaryClass nested;
    nested.name = "nested_" + std::to_string(i);
    nested.nestedObject.intField = 1000 + i;
    nested.nestedObject.stringField = "deep_" + std::to_string(i);
    original.nestedVector.push_back(nested);
  }

  // Create nested arrays
  original.nestedArrays = {{1, 2, 3}, {4, 5}, {6}};

  // Setup array object
  original.arrayObject.intVector = {10, 20, 30};
  original.arrayObject.stringVector = {"a", "b", "c"};

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  ComplexBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify nested objects
  ASSERT_EQ(original.nestedVector.size(), deserialized.nestedVector.size());
  for (size_t i = 0; i < original.nestedVector.size(); ++i) {
    EXPECT_EQ(original.nestedVector[i].name, deserialized.nestedVector[i].name);
    EXPECT_EQ(original.nestedVector[i].nestedObject.intField,
              deserialized.nestedVector[i].nestedObject.intField);
  }

  // Verify nested arrays
  EXPECT_EQ(original.nestedArrays, deserialized.nestedArrays);

  // Verify array object
  EXPECT_EQ(original.arrayObject.intVector, deserialized.arrayObject.intVector);
  EXPECT_EQ(original.arrayObject.stringVector, deserialized.arrayObject.stringVector);
}

// ================================================================================
// Portability Tests (Endianness)
// ================================================================================

TEST(BinarySerializationTest, EndianessConsistency) {
  SimpleBinaryClass original;
  original.intField = 0x12345678;  // Distinct byte pattern
  original.shortField = 0x1234;
  original.unsignedField = 0xABCDEF01;

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);
  std::string serialized = oss.str();

  // Deserialize multiple times to ensure consistency
  for (int i = 0; i < 5; ++i) {
    std::istringstream iss(serialized);
    SimpleBinaryClass deserialized;
    deserialized.deserialize(iss);

    EXPECT_EQ(original.intField, deserialized.intField);
    EXPECT_EQ(original.shortField, deserialized.shortField);
    EXPECT_EQ(original.unsignedField, deserialized.unsignedField);
  }
}

// ================================================================================
// Edge Case Tests
// ================================================================================

TEST(BinarySerializationTest, LargeData) {
  ArrayBinaryClass original;

  // Create large arrays
  for (int i = 0; i < 1000; ++i) {
    original.intVector.push_back(i);
    original.stringVector.push_back("string_" + std::to_string(i));
  }

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  ArrayBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify large arrays
  EXPECT_EQ(original.intVector, deserialized.intVector);
  EXPECT_EQ(original.stringVector, deserialized.stringVector);
}

TEST(BinarySerializationTest, FloatingPointPrecision) {
  SimpleBinaryClass original;
  original.doubleField = 3.14159265358979323846;  // High precision
  original.floatField = 2.71828f;

  // Serialize
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize
  std::istringstream iss(oss.str());
  SimpleBinaryClass deserialized;
  deserialized.deserialize(iss);

  // Verify floating point precision is maintained
  EXPECT_DOUBLE_EQ(original.doubleField, deserialized.doubleField);
  EXPECT_FLOAT_EQ(original.floatField, deserialized.floatField);
}

// ================================================================================
// Binary Format Efficiency Tests (Optional - for curiosity)
// ================================================================================

TEST(BinarySerializationTest, CompactnessComparison) {
  SimpleBinaryClass obj;
  obj.intField = 42;
  obj.stringField = "test";
  obj.doubleField = 3.14;
  obj.boolField = true;

  // Get binary size
  std::ostringstream binaryStream;
  obj.serialize(binaryStream);
  size_t binarySize = binaryStream.str().size();

  // Binary should be quite compact
  EXPECT_LT(binarySize, 200u);  // Should be much smaller than text formats
  EXPECT_GT(binarySize, 20u);   // But not unreasonably small

  std::cout << "Binary serialization size: " << binarySize << " bytes" << std::endl;
}
