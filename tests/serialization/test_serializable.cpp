#include <gtest/gtest.h>

#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "lazy/serialization/serializable.h"

/**
 * Mock serialization adapter for testing Serializable functionality
 * Records all operations for verification in tests
 */
class MockAdapter {
 public:
  struct MockNode {
    std::string key;
    std::map<std::string, MockNode*> children;
    std::vector<MockNode*> arrayElements;
    std::variant<int, double, std::string, bool> value;
    bool isObjectNode = false;
    bool isArrayNode = false;
    bool hasValue = false;

    ~MockNode() {
      for (auto& [k, child] : children) {
        delete child;
      }
      for (auto* element : arrayElements) {
        delete element;
      }
    }
  };

  using NodeType = MockNode*;

  MockAdapter() : root_(new MockNode()) {}
  MockAdapter(std::istream& /*stream*/) : root_(new MockNode()) {
    // Mock deserialization from stream - populate with test data if needed
  }
  explicit MockAdapter(std::ostream& stream) : root_(new MockNode()), writeStream_(&stream) {
    // Mock serialization to stream - for testing consistency
  }
  ~MockAdapter() { delete root_; }

  // Symmetric finish methods for new API
  void finishSerialization() const {
    if (writeStream_) {
      *writeStream_ << "mock_output";
    }
  }

  void finishDeserialization() {
    // No-op for mock adapter
  }

  NodeType root() { return root_; }

  NodeType getChild(NodeType node, const std::string& key) {
    if (key.empty()) return node;

    auto it = node->children.find(key);
    return (it != node->children.end()) ? it->second : nullptr;
  }

  NodeType addChild(NodeType node, const std::string& key) {
    if (key.empty()) return node;

    auto* child = new MockNode();
    child->key = key;
    node->children[key] = child;
    return child;
  }

  bool isObject(NodeType node) { return node->isObjectNode; }
  void setObject(NodeType node) {
    node->isObjectNode = true;
    operations_.emplace_back("setObject", node->key);
  }

  bool isArray(NodeType node) { return node->isArrayNode; }
  void setArray(NodeType node, size_t size) {
    node->isArrayNode = true;
    node->arrayElements.reserve(size);
    operations_.emplace_back("setArray", node->key + "[" + std::to_string(size) + "]");
  }

  size_t getArraySize(NodeType node) { return node->arrayElements.size(); }

  NodeType getArrayElement(NodeType node, size_t index) {
    return (index < node->arrayElements.size()) ? node->arrayElements[index] : nullptr;
  }

  NodeType addArrayElement(NodeType node) {
    auto* element = new MockNode();
    node->arrayElements.push_back(element);
    return element;
  }

  template <typename T>
  T getValue(NodeType node) {
    if (node && node->hasValue) {
      return std::get<T>(node->value);
    }
    return T{};
  }

  template <typename T>
  void setValue(NodeType node, T value) {
    node->value = value;
    node->hasValue = true;
    if constexpr (std::is_same_v<T, std::string>) {
      operations_.emplace_back("setValue", node->key + "=" + value);
    } else if constexpr (std::is_arithmetic_v<T>) {
      operations_.emplace_back("setValue", node->key + "=" + std::to_string(value));
    } else {
      operations_.emplace_back("setValue", node->key + "=(value)");
    }
  }

  // Test utilities
  const std::vector<std::pair<std::string, std::string>>& getOperations() const {
    return operations_;
  }

  void clearOperations() { operations_.clear(); }

  bool hasOperation(const std::string& op, const std::string& detail = "") {
    for (const auto& [operation, operationDetail] : operations_) {
      if (operation == op &&
          (detail.empty() || operationDetail.find(detail) != std::string::npos)) {
        return true;
      }
    }
    return false;
  }

 private:
  MockNode* root_;
  std::vector<std::pair<std::string, std::string>> operations_;
  std::ostream* writeStream_ = nullptr;  // For serialization
};

// ================================================================================
// Test Classes and Setup
// ================================================================================

// Test classes using MockAdapter
class TestClass : public lazy::serialization::Serializable<TestClass, MockAdapter> {
 public:
  SERIALIZABLE_FIELD(int, intField, 42);
  SERIALIZABLE_FIELD(std::string, stringField, "default");
  SERIALIZABLE_FIELD(double, doubleField);
};

class NestedTestClass : public lazy::serialization::Serializable<NestedTestClass, MockAdapter> {
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
SERIALIZABLE_TYPE(MockAdapter, SealedClass, value, description)
}

class TestClassWithSealed
    : public lazy::serialization::Serializable<TestClassWithSealed, MockAdapter> {
 public:
  SERIALIZABLE_FIELD(SealedClass, sealedField);
  SERIALIZABLE_FIELD(std::vector<int>, intVector);
};

class TestClassWithComplexVectors
    : public lazy::serialization::Serializable<TestClassWithComplexVectors, MockAdapter> {
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

  MockAdapter context;
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

  // Since MockAdapter doesn't actually store/retrieve data,
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
  MockAdapter context;
};

// --- Primitive Type Tests ---

TEST_F(SerializerTest, PrimitiveTypeSerialization) {
  int value = 42;
  auto node = context.root();

  lazy::serialization::Serializer<int, MockAdapter>::serialize(&value, context, node, "testInt");

  EXPECT_TRUE(context.hasOperation("setValue", "testInt=42"));
}

TEST_F(SerializerTest, StringSerialization) {
  std::string value = "hello";
  auto node = context.root();

  lazy::serialization::Serializer<std::string, MockAdapter>::serialize(&value, context, node,
                                                                       "testString");

  EXPECT_TRUE(context.hasOperation("setValue", "testString=hello"));
}

// --- Vector Type Tests ---

TEST_F(SerializerTest, VectorSerialization) {
  std::vector<int> value = {10, 20, 30};
  auto node = context.root();

  lazy::serialization::Serializer<std::vector<int>, MockAdapter>::serialize(&value, context, node,
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
  lazy::serialization::Serializer<TestClass, MockAdapter>::serialize(&value, context, node,
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
  lazy::serialization::Serializer<std::vector<TestClass>, MockAdapter>::serialize(
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
  lazy::serialization::Serializer<std::vector<SealedClass>, MockAdapter>::serialize(
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

  MockAdapter context;
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
