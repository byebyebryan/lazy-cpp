#include <gtest/gtest.h>

#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

#include "lazy/serialization/serializable.h"

// ================================================================================
// MOCK ADAPTER FOR TESTING SERIALIZABLE AND SERIALIZER CORE FUNCTIONALITY
//
// This test file focuses on testing the core Serializable framework and Serializer
// component logic using a MockAdapter. It does NOT test specific format serialization
// (that's handled in adapter-specific test files).
// ================================================================================

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
    } else if constexpr (std::is_same_v<T, bool>) {
      operations_.emplace_back("setValue", node->key + "=(value)");
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
// Basic Serializable Framework Tests
// ================================================================================

class SerializableTest : public ::testing::Test {
 protected:
  void SetUp() override { context.clearOperations(); }

  MockAdapter context;
};

TEST_F(SerializableTest, SerializableFieldRegistration) {
  // Test that fields are properly registered and serialized through public interface
  TestClass obj;
  obj.intField = 999;
  obj.stringField = "registration_test";
  obj.doubleField = 1.23;

  // Test that the public serialization interface works (which relies on field registration)
  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));

  // Should produce mock output indicating serialization occurred
  EXPECT_EQ(oss.str(), "mock_output");
}

TEST_F(SerializableTest, SerializationInterface) {
  TestClass obj;
  obj.intField = 100;
  obj.stringField = "test";
  obj.doubleField = 3.14;

  // Test that the public serialization interface works
  std::ostringstream oss;
  obj.serialize(oss);
  std::string output = oss.str();

  // MockAdapter should produce mock output
  EXPECT_EQ(output, "mock_output");
}

TEST_F(SerializableTest, DeserializationInterface) {
  TestClass obj;

  // Test that the public deserialization interface works without crashing
  std::istringstream iss("mock_input");
  EXPECT_NO_THROW(obj.deserialize(iss));
}

TEST_F(SerializableTest, NestedObjectSerialization) {
  // Test that nested Serializable objects work correctly through public interface
  NestedTestClass obj;
  obj.name = "parent";
  obj.nestedObject.intField = 555;
  obj.nestedObject.stringField = "nested_value";

  // Test that nested objects serialize without crashing
  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));
  EXPECT_EQ(oss.str(), "mock_output");
}

TEST_F(SerializableTest, DefaultValues) {
  // Test that default values work through serialization interface
  TestClass obj;  // Should have defaults: intField=42, stringField="default", doubleField=0.0

  // Verify default values are set correctly
  EXPECT_EQ(obj.intField, 42);
  EXPECT_EQ(obj.stringField, "default");
  EXPECT_DOUBLE_EQ(obj.doubleField, 0.0);

  // Test that defaults serialize without issues
  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));
  EXPECT_EQ(oss.str(), "mock_output");
}

// ================================================================================
// Serializer Component Tests
//
// These tests verify the core Serializer template specializations work correctly
// with different data types. They focus on the dispatching logic and type handling.
// ================================================================================

class SerializerTest : public ::testing::Test {
 protected:
  MockAdapter context;
};

TEST_F(SerializerTest, PrimitiveTypeDispatching) {
  // Test that primitive types are correctly dispatched to setValue
  int intValue = 42;
  std::string stringValue = "hello";
  double doubleValue = 3.14;
  bool boolValue = true;

  auto node = context.root();

  lazy::serialization::Serializer<int, MockAdapter>::serialize(&intValue, context, node,
                                                               "intField");
  lazy::serialization::Serializer<std::string, MockAdapter>::serialize(&stringValue, context, node,
                                                                       "stringField");
  lazy::serialization::Serializer<double, MockAdapter>::serialize(&doubleValue, context, node,
                                                                  "doubleField");
  lazy::serialization::Serializer<bool, MockAdapter>::serialize(&boolValue, context, node,
                                                                "boolField");

  // Verify correct setValue operations
  EXPECT_TRUE(context.hasOperation("setValue", "intField=42"));
  EXPECT_TRUE(context.hasOperation("setValue", "stringField=hello"));
  EXPECT_TRUE(context.hasOperation("setValue", "doubleField=3.14"));
  // Bool serialization - value format may vary
  EXPECT_TRUE(context.hasOperation("setValue", "boolField") ||
              context.hasOperation("setValue", "boolField=true") ||
              context.hasOperation("setValue", "boolField=1"));
}

TEST_F(SerializerTest, VectorTypeDispatching) {
  // Test that vector types are correctly dispatched to array operations
  std::vector<int> intVector = {1, 2, 3};
  auto node = context.root();

  lazy::serialization::Serializer<std::vector<int>, MockAdapter>::serialize(&intVector, context,
                                                                            node, "testVector");

  // Should set up array and serialize elements
  EXPECT_TRUE(context.hasOperation("setArray", "[3]"));
  EXPECT_TRUE(context.hasOperation("setValue", "=1"));
  EXPECT_TRUE(context.hasOperation("setValue", "=2"));
  EXPECT_TRUE(context.hasOperation("setValue", "=3"));
}

TEST_F(SerializerTest, SerializableTypeDispatching) {
  // Test that Serializable-derived types are correctly dispatched to object operations
  TestClass obj;
  obj.intField = 777;
  obj.stringField = "test_object";

  auto node = context.root();
  lazy::serialization::Serializer<TestClass, MockAdapter>::serialize(&obj, context, node,
                                                                     "testObject");

  // Should set up object and serialize fields
  EXPECT_TRUE(context.hasOperation("setObject"));
  EXPECT_TRUE(context.hasOperation("setValue", "intField=777"));
  EXPECT_TRUE(context.hasOperation("setValue", "stringField=test_object"));
}

TEST_F(SerializerTest, ExternalTypeDispatching) {
  // Test that external types (using SERIALIZABLE_TYPE) work correctly
  SealedClass obj;
  obj.value = 123;
  obj.description = "external_test";

  auto node = context.root();
  lazy::serialization::Serializer<SealedClass, MockAdapter>::serialize(&obj, context, node,
                                                                       "externalObject");

  // Should use SERIALIZABLE_TYPE specialization
  EXPECT_TRUE(context.hasOperation("setObject"));
  EXPECT_TRUE(context.hasOperation("setValue", "value=123"));
  EXPECT_TRUE(context.hasOperation("setValue", "description=external_test"));
}

TEST_F(SerializerTest, NestedVectorSerialization) {
  // Test one complex case to verify nested serialization works
  std::vector<TestClass> nestedVector;

  TestClass item1;
  item1.intField = 100;
  item1.stringField = "item1";

  TestClass item2;
  item2.intField = 200;
  item2.stringField = "item2";

  nestedVector = {item1, item2};

  auto node = context.root();
  lazy::serialization::Serializer<std::vector<TestClass>, MockAdapter>::serialize(
      &nestedVector, context, node, "nestedVector");

  // Should set up array with objects
  EXPECT_TRUE(context.hasOperation("setArray", "[2]"));
  EXPECT_TRUE(context.hasOperation("setObject"));
  EXPECT_TRUE(context.hasOperation("setValue", "intField=100"));
  EXPECT_TRUE(context.hasOperation("setValue", "stringField=item1"));
  EXPECT_TRUE(context.hasOperation("setValue", "intField=200"));
  EXPECT_TRUE(context.hasOperation("setValue", "stringField=item2"));
}

// ================================================================================
// SERIALIZABLE_TYPE Macro Tests
//
// Tests for the SERIALIZABLE_TYPE macro functionality with external/sealed classes
// ================================================================================

class SerializableTypeTest : public ::testing::Test {
 protected:
  MockAdapter context;
};

TEST_F(SerializableTypeTest, ExternalClassSerialization) {
  // Test that SERIALIZABLE_TYPE macro works with external classes
  TestClassWithSealed obj;
  obj.sealedField.value = 999;
  obj.sealedField.description = "external_field_test";
  obj.intVector = {1, 2, 3};

  // Test that external classes serialize through public interface
  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));
  EXPECT_EQ(oss.str(), "mock_output");
}

TEST_F(SerializableTypeTest, ComplexVectorSerialization) {
  // Test complex vectors with various types through public interface
  TestClassWithComplexVectors obj;
  obj.description = "complex_test";

  // Add nested Serializable objects
  TestClass nested;
  nested.intField = 100;
  nested.stringField = "nested_item";
  obj.nestedVector = {nested};

  // Add external/sealed objects
  SealedClass sealed;
  sealed.value = 200;
  sealed.description = "sealed_item";
  obj.sealedVector = {sealed};

  // Test that complex structures serialize without issues
  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));
  EXPECT_EQ(oss.str(), "mock_output");
}

// ================================================================================
// Integration and Edge Case Tests
// ================================================================================

class SerializableIntegrationTest : public ::testing::Test {
 protected:
  MockAdapter context;
};

TEST_F(SerializableIntegrationTest, StreamIntegration) {
  // Test that the stream integration works end-to-end
  TestClass obj;
  obj.intField = 42;
  obj.stringField = "integration_test";

  // Test serialization interface
  std::ostringstream oss;
  obj.serialize(oss);
  EXPECT_EQ(oss.str(), "mock_output");

  // Test deserialization interface
  std::istringstream iss("mock_input");
  EXPECT_NO_THROW(obj.deserialize(iss));
}

TEST_F(SerializableIntegrationTest, EmptyVectorsHandling) {
  // Test that empty vectors are handled correctly
  TestClassWithComplexVectors obj;
  obj.description = "empty_vectors_test";
  // nestedVector and sealedVector remain empty

  // Verify empty vectors don't cause issues
  EXPECT_TRUE(obj.nestedVector.empty());
  EXPECT_TRUE(obj.sealedVector.empty());

  // Test that empty vectors serialize without crashing
  std::ostringstream oss;
  EXPECT_NO_THROW(obj.serialize(oss));
  EXPECT_EQ(oss.str(), "mock_output");
}
