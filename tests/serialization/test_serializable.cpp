#include <gtest/gtest.h>

#include "lazy/serialization/serializable.h"
#include "mock_context.h"

// ================================================================================
// Test Classes and Setup
// ================================================================================

// Test classes using MockContext
class TestClass : public lazy::serialization::Serializable<TestClass, MockContext> {
 public:
  SERIALIZABLE_FIELD(int, intField, 42);
  SERIALIZABLE_FIELD(std::string, stringField, "default");
  SERIALIZABLE_FIELD(double, doubleField);
};

class NestedTestClass : public lazy::serialization::Serializable<NestedTestClass, MockContext> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "nested");
  SERIALIZABLE_FIELD(TestClass, nestedObject);
};

// Test external/sealed class with SERIALIZABLE_TYPE
class SealedClass {
 public:
  int value = 123;
  std::string description = "sealed";
};

namespace lazy::serialization {
SERIALIZABLE_TYPE(MockContext, SealedClass, value, description)
}

class TestClassWithSealed
    : public lazy::serialization::Serializable<TestClassWithSealed, MockContext> {
 public:
  SERIALIZABLE_FIELD(SealedClass, sealedField);
  SERIALIZABLE_FIELD(std::vector<int>, intVector);
};

class TestClassWithComplexVectors
    : public lazy::serialization::Serializable<TestClassWithComplexVectors, MockContext> {
 public:
  SERIALIZABLE_FIELD(std::vector<TestClass>, nestedVector);
  SERIALIZABLE_FIELD(std::vector<SealedClass>, sealedVector);
  SERIALIZABLE_FIELD(std::string, description, "complex_vectors");
};

// ================================================================================
// Basic Serializable Functionality Tests
// ================================================================================

class SerializableTest : public ::testing::Test {
 protected:
  void SetUp() override { context.clearOperations(); }

  MockContext context;
};

TEST_F(SerializableTest, BasicSerialization) {
  TestClass obj;
  obj.intField = 100;
  obj.stringField = "test";
  obj.doubleField = 3.14;

  // Test serialization produces output
  std::ostringstream oss;
  obj.serialize(oss);
  std::string output = oss.str();

  EXPECT_EQ(output, "mock_output");
}

TEST_F(SerializableTest, DefaultValues) {
  TestClass obj;

  std::ostringstream oss;
  obj.serialize(oss);

  // Should produce output
  EXPECT_FALSE(oss.str().empty());
}

TEST_F(SerializableTest, SerializationProducesOutput) {
  TestClass obj;
  obj.intField = 123;
  obj.stringField = "hello";

  std::ostringstream oss;
  obj.serialize(oss);

  // Should produce some output
  EXPECT_FALSE(oss.str().empty());
}

TEST_F(SerializableTest, DeserializationWorks) {
  TestClass original;
  original.intField = 456;
  original.stringField = "deserialization_test";

  // Serialize to stream
  std::ostringstream oss;
  original.serialize(oss);

  // Deserialize from stream
  TestClass deserialized;
  std::istringstream iss(oss.str());
  deserialized.deserialize(iss);

  // Since MockContext doesn't actually store/retrieve data,
  // we just test that deserialization doesn't crash
  EXPECT_NO_THROW(deserialized.deserialize(iss));
}

TEST_F(SerializableTest, NestedObjectInstantiation) {
  // Test that nested objects can be created and used
  NestedTestClass obj;
  obj.name = "parent";
  obj.nestedObject.intField = 555;
  obj.nestedObject.stringField = "nested_value";

  // Test that we can serialize without crashing
  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));
}

// ================================================================================
// Serializer Component Tests
// ================================================================================

class SerializerTest : public ::testing::Test {
 protected:
  MockContext context;
};

// --- Primitive Type Tests ---

TEST_F(SerializerTest, PrimitiveTypeSerialization) {
  int value = 42;
  auto node = context.root();

  lazy::serialization::Serializer<int, MockContext>::serialize(&value, context, node, "testInt");

  EXPECT_TRUE(context.hasOperation("setValue", "testInt=42"));
}

TEST_F(SerializerTest, StringSerialization) {
  std::string value = "hello";
  auto node = context.root();

  lazy::serialization::Serializer<std::string, MockContext>::serialize(&value, context, node,
                                                                       "testString");

  EXPECT_TRUE(context.hasOperation("setValue", "testString=hello"));
}

// --- Vector Type Tests ---

TEST_F(SerializerTest, VectorSerialization) {
  std::vector<int> value = {10, 20, 30};
  auto node = context.root();

  lazy::serialization::Serializer<std::vector<int>, MockContext>::serialize(&value, context, node,
                                                                            "testVector");

  EXPECT_TRUE(context.hasOperation("setArray", "[3]"));
  // Vector elements have empty keys in array serialization
  EXPECT_TRUE(context.hasOperation("setValue", "=10"));
  EXPECT_TRUE(context.hasOperation("setValue", "=20"));
  EXPECT_TRUE(context.hasOperation("setValue", "=30"));
}

// --- Complex Object Tests ---

TEST_F(SerializerTest, SerializableObjectSerialization) {
  TestClass value;
  value.intField = 777;
  value.stringField = "serializer_test";

  auto node = context.root();
  lazy::serialization::Serializer<TestClass, MockContext>::serialize(&value, context, node,
                                                                     "testObject");

  EXPECT_TRUE(context.hasOperation("setObject"));
  EXPECT_TRUE(context.hasOperation("setValue", "intField=777"));
  EXPECT_TRUE(context.hasOperation("setValue", "stringField=serializer_test"));
}

TEST_F(SerializerTest, VectorOfNestedObjectsSerialization) {
  std::vector<TestClass> value;

  TestClass obj1;
  obj1.intField = 100;
  obj1.stringField = "first";

  TestClass obj2;
  obj2.intField = 200;
  obj2.stringField = "second";

  value = {obj1, obj2};

  auto node = context.root();
  lazy::serialization::Serializer<std::vector<TestClass>, MockContext>::serialize(
      &value, context, node, "testNestedVector");

  EXPECT_TRUE(context.hasOperation("setArray", "[2]"));
  EXPECT_TRUE(context.hasOperation("setObject"));
  EXPECT_TRUE(context.hasOperation("setValue", "intField=100"));
  EXPECT_TRUE(context.hasOperation("setValue", "stringField=first"));
  EXPECT_TRUE(context.hasOperation("setValue", "intField=200"));
  EXPECT_TRUE(context.hasOperation("setValue", "stringField=second"));
}

TEST_F(SerializerTest, VectorOfCustomObjectsSerialization) {
  std::vector<SealedClass> value;

  SealedClass custom1;
  custom1.value = 42;
  custom1.description = "first_custom";

  SealedClass custom2;
  custom2.value = 84;
  custom2.description = "second_custom";

  value = {custom1, custom2};

  auto node = context.root();
  lazy::serialization::Serializer<std::vector<SealedClass>, MockContext>::serialize(
      &value, context, node, "testCustomVector");

  EXPECT_TRUE(context.hasOperation("setArray", "[2]"));
  EXPECT_TRUE(context.hasOperation("setObject"));
  EXPECT_TRUE(context.hasOperation("setValue", "value=42"));
  EXPECT_TRUE(context.hasOperation("setValue", "description=first_custom"));
  EXPECT_TRUE(context.hasOperation("setValue", "value=84"));
  EXPECT_TRUE(context.hasOperation("setValue", "description=second_custom"));
}

// ================================================================================
// Complex Vector Serialization Tests
// ================================================================================

class ComplexVectorTest : public ::testing::Test {
 protected:
  void SetUp() override { context.clearOperations(); }

  MockContext context;
};

TEST_F(ComplexVectorTest, SerializationWithComplexVectors) {
  TestClassWithComplexVectors obj;

  // Add nested objects
  TestClass nested1;
  nested1.intField = 1;
  nested1.stringField = "nested_one";

  TestClass nested2;
  nested2.intField = 2;
  nested2.stringField = "nested_two";

  obj.nestedVector = {nested1, nested2};

  // Add sealed objects
  SealedClass sealed1;
  sealed1.value = 10;
  sealed1.description = "sealed_one";

  obj.sealedVector = {sealed1};
  obj.description = "test_complex_vectors";

  std::ostringstream oss;
  obj.serialize(oss);

  // Should produce output
  EXPECT_FALSE(oss.str().empty());
}

TEST_F(ComplexVectorTest, EmptyComplexVectors) {
  TestClassWithComplexVectors obj;
  // Keep vectors empty, only set description
  obj.description = "empty_vectors";

  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));
  EXPECT_FALSE(oss.str().empty());
}
