#include <gtest/gtest.h>

#include <sstream>

#include "lazy/serialization/text.h"

// ================================================================================
// Test Classes and Setup
// ================================================================================

class SimpleTextClass : public lazy::TextSerializable<SimpleTextClass> {
 public:
  SERIALIZABLE_FIELD(int, intField, 42);
  SERIALIZABLE_FIELD(std::string, stringField, "default");
  SERIALIZABLE_FIELD(double, doubleField, 3.14);
  SERIALIZABLE_FIELD(bool, boolField, true);
};

class NestedTextClass : public lazy::TextSerializable<NestedTextClass> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "nested");
  SERIALIZABLE_FIELD(SimpleTextClass, nestedObject);
};

class ArrayTextClass : public lazy::TextSerializable<ArrayTextClass> {
 public:
  SERIALIZABLE_FIELD(std::vector<int>, intVector);
  SERIALIZABLE_FIELD(std::vector<std::string>, stringVector);
  SERIALIZABLE_FIELD(std::vector<SimpleTextClass>, objectVector);
};

// External class for SERIALIZABLE_TYPE testing
class ExternalClass {
 public:
  int value = 123;
  std::string description = "external";
  bool flag = false;
};

namespace lazy::serialization {
SERIALIZABLE_TYPE(TextContext, ExternalClass, value, description, flag)
}

class TextClassWithExternal : public lazy::TextSerializable<TextClassWithExternal> {
 public:
  SERIALIZABLE_FIELD(ExternalClass, externalField);
  SERIALIZABLE_FIELD(std::string, name, "with_external");
};

// ================================================================================
// TextContext Unit Tests
// ================================================================================

class TextContextTest : public ::testing::Test {
 protected:
  lazy::serialization::TextContext context;
};

TEST_F(TextContextTest, BasicValueOperations) {
  // Test setValue and getValue for different types
  auto root = context.root();
  auto intNode = context.addChild(root, "intKey");
  auto stringNode = context.addChild(root, "stringKey");
  auto doubleNode = context.addChild(root, "doubleKey");
  auto boolNode = context.addChild(root, "boolKey");

  context.setValue(intNode, 42);
  context.setValue(stringNode, std::string("hello"));
  context.setValue(doubleNode, 3.14);
  context.setValue(boolNode, true);

  EXPECT_EQ(context.getValue<int>(intNode), 42);
  EXPECT_EQ(context.getValue<std::string>(stringNode), "hello");
  EXPECT_DOUBLE_EQ(context.getValue<double>(doubleNode), 3.14);
  EXPECT_EQ(context.getValue<bool>(boolNode), true);
}

TEST_F(TextContextTest, NodePathOperations) {
  auto root = context.root();
  EXPECT_EQ(*root, "");

  auto child = context.addChild(root, "parent");
  EXPECT_EQ(*child, "parent");

  auto grandchild = context.addChild(child, "child");
  EXPECT_EQ(*grandchild, "parent.child");

  // Add some data so getChild can find the node
  context.setValue(child, std::string("test"));
  auto retrieved = context.getChild(root, "parent");
  EXPECT_NE(retrieved, nullptr);
  EXPECT_EQ(*retrieved, "parent");
}

TEST_F(TextContextTest, ArrayOperations) {
  auto root = context.root();
  auto arrayNode = context.addChild(root, "testArray");

  context.setArray(arrayNode, 3);
  EXPECT_TRUE(context.isArray(arrayNode));
  EXPECT_EQ(context.getArraySize(arrayNode), 3);

  auto element0 = context.getArrayElement(arrayNode, 0);
  EXPECT_EQ(*element0, "testArray.0");

  auto element1 = context.getArrayElement(arrayNode, 1);
  EXPECT_EQ(*element1, "testArray.1");
}

TEST_F(TextContextTest, StreamRoundTrip) {
  // Set some values
  auto root = context.root();
  auto key1Node = context.addChild(root, "key1");
  auto key2Node = context.addChild(root, "key2");
  auto nestedParent = context.addChild(root, "nested");
  auto nestedField = context.addChild(nestedParent, "field");

  context.setValue(key1Node, std::string("value1"));
  context.setValue(key2Node, 42);
  context.setValue(nestedField, 3.14);

  // Since this fixture uses default context (no stream), we need to create new context for
  // serialization
  std::stringstream ss;
  lazy::serialization::TextContext writeContext(static_cast<std::ostream&>(ss));
  auto writeRoot = writeContext.root();
  auto writeKey1Node = writeContext.addChild(writeRoot, "key1");
  auto writeKey2Node = writeContext.addChild(writeRoot, "key2");
  auto writeNestedParent = writeContext.addChild(writeRoot, "nested");
  auto writeNestedField = writeContext.addChild(writeNestedParent, "field");

  writeContext.setValue(writeKey1Node, std::string("value1"));
  writeContext.setValue(writeKey2Node, 42);
  writeContext.setValue(writeNestedField, 3.14);
  writeContext.finishSerialization();  // Ensure all data is flushed

  // Deserialize from stream
  lazy::serialization::TextContext context2(static_cast<std::istream&>(ss));

  // Verify values are preserved by getting nodes and checking their values
  auto root2 = context2.root();
  auto key1Node2 = context2.getChild(root2, "key1");
  auto key2Node2 = context2.getChild(root2, "key2");
  auto nestedParent2 = context2.getChild(root2, "nested");
  auto nestedField2 = context2.getChild(nestedParent2, "field");

  EXPECT_NE(key1Node2, nullptr);
  EXPECT_NE(key2Node2, nullptr);
  EXPECT_NE(nestedField2, nullptr);

  EXPECT_EQ(context2.getValue<std::string>(key1Node2), "value1");
  EXPECT_EQ(context2.getValue<int>(key2Node2), 42);
  EXPECT_DOUBLE_EQ(context2.getValue<double>(nestedField2), 3.14);
}

// ================================================================================
// Simple Serializable Tests
// ================================================================================

class SimpleTextSerializationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    obj.intField = 100;
    obj.stringField = "test";
    obj.doubleField = 2.71;
    obj.boolField = false;
  }

  SimpleTextClass obj;
};

TEST_F(SimpleTextSerializationTest, SerializationOutput) {
  std::stringstream ss;
  obj.serialize(ss);
  std::string output = ss.str();

  // Verify the output contains expected key-value pairs
  EXPECT_TRUE(output.find("intField = 100") != std::string::npos);
  EXPECT_TRUE(output.find("stringField = \"test\"") != std::string::npos);
  EXPECT_TRUE(output.find("doubleField = 2.71") != std::string::npos);
  EXPECT_TRUE(output.find("boolField = false") != std::string::npos);
}

TEST_F(SimpleTextSerializationTest, DeserializationRoundTrip) {
  // Serialize
  std::stringstream ss;
  obj.serialize(ss);

  // Deserialize
  SimpleTextClass obj2;
  ss.seekg(0);
  obj2.deserialize(ss);

  // Verify fields match
  EXPECT_EQ(obj2.intField, obj.intField);
  EXPECT_EQ(obj2.stringField, obj.stringField);
  EXPECT_DOUBLE_EQ(obj2.doubleField, obj.doubleField);
  EXPECT_EQ(obj2.boolField, obj.boolField);
}

TEST_F(SimpleTextSerializationTest, DefaultValues) {
  SimpleTextClass defaultObj;

  std::stringstream ss;
  defaultObj.serialize(ss);
  std::string output = ss.str();

  EXPECT_TRUE(output.find("intField = 42") != std::string::npos);
  EXPECT_TRUE(output.find("stringField = \"default\"") != std::string::npos);
  EXPECT_TRUE(output.find("doubleField = 3.14") != std::string::npos);
  EXPECT_TRUE(output.find("boolField = true") != std::string::npos);
}

// ================================================================================
// Nested Object Tests
// ================================================================================

class NestedTextSerializationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    obj.name = "parent";
    obj.nestedObject.intField = 555;
    obj.nestedObject.stringField = "nested_value";
    obj.nestedObject.doubleField = 1.41;
    obj.nestedObject.boolField = true;
  }

  NestedTextClass obj;
};

TEST_F(NestedTextSerializationTest, NestedSerializationOutput) {
  std::stringstream ss;
  obj.serialize(ss);
  std::string output = ss.str();

  // Verify nested structure with dot notation
  EXPECT_TRUE(output.find("name = \"parent\"") != std::string::npos);
  EXPECT_TRUE(output.find("nestedObject.intField = 555") != std::string::npos);
  EXPECT_TRUE(output.find("nestedObject.stringField = \"nested_value\"") != std::string::npos);
  EXPECT_TRUE(output.find("nestedObject.doubleField = 1.41") != std::string::npos);
  EXPECT_TRUE(output.find("nestedObject.boolField = true") != std::string::npos);
}

TEST_F(NestedTextSerializationTest, NestedDeserializationRoundTrip) {
  // Serialize
  std::stringstream ss;
  obj.serialize(ss);

  // Deserialize
  NestedTextClass obj2;
  ss.seekg(0);
  obj2.deserialize(ss);

  // Verify nested fields match
  EXPECT_EQ(obj2.name, obj.name);
  EXPECT_EQ(obj2.nestedObject.intField, obj.nestedObject.intField);
  EXPECT_EQ(obj2.nestedObject.stringField, obj.nestedObject.stringField);
  EXPECT_DOUBLE_EQ(obj2.nestedObject.doubleField, obj.nestedObject.doubleField);
  EXPECT_EQ(obj2.nestedObject.boolField, obj.nestedObject.boolField);
}

// ================================================================================
// Array Tests
// ================================================================================

class ArrayTextSerializationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    obj.intVector = {10, 20, 30};
    obj.stringVector = {"hello", "world", "test"};

    // Add object vector
    SimpleTextClass item1;
    item1.intField = 1;
    item1.stringField = "first";

    SimpleTextClass item2;
    item2.intField = 2;
    item2.stringField = "second";

    obj.objectVector = {item1, item2};
  }

  ArrayTextClass obj;
};

TEST_F(ArrayTextSerializationTest, ArraySerializationOutput) {
  std::stringstream ss;
  obj.serialize(ss);
  std::string output = ss.str();

  // Verify array count and elements
  EXPECT_TRUE(output.find("intVector.count = 3") != std::string::npos);
  EXPECT_TRUE(output.find("intVector.0 = 10") != std::string::npos);
  EXPECT_TRUE(output.find("intVector.1 = 20") != std::string::npos);
  EXPECT_TRUE(output.find("intVector.2 = 30") != std::string::npos);

  EXPECT_TRUE(output.find("stringVector.count = 3") != std::string::npos);
  EXPECT_TRUE(output.find("stringVector.0 = \"hello\"") != std::string::npos);
  EXPECT_TRUE(output.find("stringVector.1 = \"world\"") != std::string::npos);
  EXPECT_TRUE(output.find("stringVector.2 = \"test\"") != std::string::npos);

  EXPECT_TRUE(output.find("objectVector.count = 2") != std::string::npos);
  EXPECT_TRUE(output.find("objectVector.0.intField = 1") != std::string::npos);
  EXPECT_TRUE(output.find("objectVector.0.stringField = \"first\"") != std::string::npos);
  EXPECT_TRUE(output.find("objectVector.1.intField = 2") != std::string::npos);
  EXPECT_TRUE(output.find("objectVector.1.stringField = \"second\"") != std::string::npos);
}

TEST_F(ArrayTextSerializationTest, ArrayDeserializationRoundTrip) {
  // Serialize
  std::stringstream ss;
  obj.serialize(ss);

  // Deserialize
  ArrayTextClass obj2;
  ss.seekg(0);
  obj2.deserialize(ss);

  // Verify int vector
  ASSERT_EQ(obj2.intVector.size(), 3);
  EXPECT_EQ(obj2.intVector[0], 10);
  EXPECT_EQ(obj2.intVector[1], 20);
  EXPECT_EQ(obj2.intVector[2], 30);

  // Verify string vector
  ASSERT_EQ(obj2.stringVector.size(), 3);
  EXPECT_EQ(obj2.stringVector[0], "hello");
  EXPECT_EQ(obj2.stringVector[1], "world");
  EXPECT_EQ(obj2.stringVector[2], "test");

  // Verify object vector
  ASSERT_EQ(obj2.objectVector.size(), 2);
  EXPECT_EQ(obj2.objectVector[0].intField, 1);
  EXPECT_EQ(obj2.objectVector[0].stringField, "first");
  EXPECT_EQ(obj2.objectVector[1].intField, 2);
  EXPECT_EQ(obj2.objectVector[1].stringField, "second");
}

TEST_F(ArrayTextSerializationTest, EmptyArrays) {
  ArrayTextClass emptyObj;

  std::stringstream ss;
  emptyObj.serialize(ss);
  std::string output = ss.str();

  // Empty arrays should serialize with count = 0
  EXPECT_TRUE(output.find("intVector.count = 0") != std::string::npos);
  EXPECT_TRUE(output.find("stringVector.count = 0") != std::string::npos);
  EXPECT_TRUE(output.find("objectVector.count = 0") != std::string::npos);

  // Deserialize should work
  ArrayTextClass emptyObj2;
  ss.seekg(0);
  EXPECT_NO_THROW(emptyObj2.deserialize(ss));

  EXPECT_EQ(emptyObj2.intVector.size(), 0);
  EXPECT_EQ(emptyObj2.stringVector.size(), 0);
  EXPECT_EQ(emptyObj2.objectVector.size(), 0);
}

// ================================================================================
// External Type Tests (SERIALIZABLE_TYPE)
// ================================================================================

class ExternalTextSerializationTest : public ::testing::Test {
 protected:
  void SetUp() override {
    obj.externalField.value = 999;
    obj.externalField.description = "test_external";
    obj.externalField.flag = true;
    obj.name = "wrapper";
  }

  TextClassWithExternal obj;
};

TEST_F(ExternalTextSerializationTest, ExternalTypeSerializationOutput) {
  std::stringstream ss;
  obj.serialize(ss);
  std::string output = ss.str();

  EXPECT_TRUE(output.find("name = \"wrapper\"") != std::string::npos);
  EXPECT_TRUE(output.find("externalField.value = 999") != std::string::npos);
  EXPECT_TRUE(output.find("externalField.description = \"test_external\"") != std::string::npos);
  EXPECT_TRUE(output.find("externalField.flag = true") != std::string::npos);
}

TEST_F(ExternalTextSerializationTest, ExternalTypeDeserializationRoundTrip) {
  // Serialize
  std::stringstream ss;
  obj.serialize(ss);

  // Deserialize
  TextClassWithExternal obj2;
  ss.seekg(0);
  obj2.deserialize(ss);

  // Verify external fields match
  EXPECT_EQ(obj2.name, obj.name);
  EXPECT_EQ(obj2.externalField.value, obj.externalField.value);
  EXPECT_EQ(obj2.externalField.description, obj.externalField.description);
  EXPECT_EQ(obj2.externalField.flag, obj.externalField.flag);
}

// ================================================================================
// Manual Format Parsing Tests
// ================================================================================

class TextFormatParsingTest : public ::testing::Test {};

TEST_F(TextFormatParsingTest, ManualFormatDeserialization) {
  // Create a manual text format string
  std::stringstream input;
  input << "intField = 777\n";
  input << "stringField = \"manual_test\"\n";
  input << "doubleField = 9.99\n";
  input << "boolField = false\n";

  // Deserialize
  SimpleTextClass obj;
  input.seekg(0);
  obj.deserialize(input);

  EXPECT_EQ(obj.intField, 777);
  EXPECT_EQ(obj.stringField, "manual_test");
  EXPECT_DOUBLE_EQ(obj.doubleField, 9.99);
  EXPECT_EQ(obj.boolField, false);
}

TEST_F(TextFormatParsingTest, ComplexManualFormat) {
  std::stringstream input;
  input << "name = \"complex_test\"\n";
  input << "nestedObject.intField = 123\n";
  input << "nestedObject.stringField = \"nested\"\n";
  input << "nestedObject.doubleField = 1.23\n";
  input << "nestedObject.boolField = true\n";

  NestedTextClass obj;
  input.seekg(0);
  obj.deserialize(input);

  EXPECT_EQ(obj.name, "complex_test");
  EXPECT_EQ(obj.nestedObject.intField, 123);
  EXPECT_EQ(obj.nestedObject.stringField, "nested");
  EXPECT_DOUBLE_EQ(obj.nestedObject.doubleField, 1.23);
  EXPECT_EQ(obj.nestedObject.boolField, true);
}

TEST_F(TextFormatParsingTest, ArrayManualFormat) {
  std::stringstream input;
  input << "intVector.count = 2\n";
  input << "intVector.0 = 100\n";
  input << "intVector.1 = 200\n";
  input << "stringVector.count = 1\n";
  input << "stringVector.0 = \"single\"\n";
  input << "objectVector.count = 0\n";

  ArrayTextClass obj;
  input.seekg(0);
  obj.deserialize(input);

  ASSERT_EQ(obj.intVector.size(), 2);
  EXPECT_EQ(obj.intVector[0], 100);
  EXPECT_EQ(obj.intVector[1], 200);

  ASSERT_EQ(obj.stringVector.size(), 1);
  EXPECT_EQ(obj.stringVector[0], "single");

  EXPECT_EQ(obj.objectVector.size(), 0);
}
