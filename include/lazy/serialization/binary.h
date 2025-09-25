#pragma once

#include <endian.h>

#include <cstring>
#include <istream>
#include <ostream>
#include <string>
#include <type_traits>
#include <vector>

#include "lazy/serialization/serializable.h"

namespace lazy {
namespace serialization {

/**
 * Simple binary serialization context implementation.
 * Provides the ContextType interface required by Serializable.
 *
 * Binary Memory Layout:
 * - Primitives: stored as little-endian bytes (int32 = 4 bytes, double = 8 bytes, etc.)
 * - Strings: [length:uint32][bytes...] (length-prefixed, no null terminator)
 * - Arrays: [count:uint32][element][element]... (count-prefixed elements)
 * - Objects: fields serialized sequentially in declaration order
 * - Booleans: single byte (0x00 = false, 0x01 = true)
 *
 * Features:
 * - Simple sequential binary format
 * - Portable (handles endianness via htole32/le32toh)
 * - Direct stream writing: setValue() writes immediately to output stream
 * - No intermediate data collection needed
 * - Sequential reading during deserialization directly from input stream
 */
class BinaryContext {
 private:
  std::istream* readStream_;   // For deserialization
  std::ostream* writeStream_;  // For serialization

  // Interface compatibility - framework needs valid pointer, we don't use the string value
  mutable std::string dummyPath_;

 public:
  using NodeType = const std::string*;  // Interface compatibility - we don't use hierarchical nodes
                                        // in binary format

  // For deserialization
  explicit BinaryContext(std::istream& stream)
      : readStream_(&stream), writeStream_(nullptr), dummyPath_("") {}

  // For serialization
  explicit BinaryContext(std::ostream& stream)
      : readStream_(nullptr), writeStream_(&stream), dummyPath_("") {}

  // Clean finish methods for symmetric API
  void finishSerialization() {
    // No-op for binary - already writing directly to stream during setValue()
  }

  void finishDeserialization() {
    // No-op for binary - no cleanup needed after deserialization
  }

  NodeType root() {
    return &dummyPath_;  // Framework needs valid pointer - we don't use hierarchical structure
  }

  NodeType getChild(NodeType node, const std::string& key) {
    return &dummyPath_;  // Framework needs valid pointer - binary format is sequential
  }

  NodeType addChild(NodeType node, const std::string& key) {
    return &dummyPath_;  // Framework needs valid pointer - binary format is sequential
  }

  bool isObject(NodeType node) {
    return true;  // Allow serializable framework to iterate fields
  }

  void setObject(NodeType node) {
    // No special object setup needed
  }

  bool isArray(NodeType node) {
    return true;  // Allow arrays
  }

  void setArray(NodeType node, size_t size) {
    // Write array size directly to stream
    if (writeStream_) {
      writeUInt32ToStream(static_cast<uint32_t>(size));
    }
  }

  size_t getArraySize(NodeType node) { return readUInt32(); }

  NodeType getArrayElement(NodeType node, size_t index) { return node; }

  NodeType addArrayElement(NodeType node) { return node; }

  // Value serialization/deserialization with endianness handling
  template <typename T>
  T getValue(NodeType node) {
    if (!readStream_) return T{};

    if constexpr (std::is_same_v<T, std::string>) {
      return readString();
    } else if constexpr (std::is_same_v<T, bool>) {
      uint8_t value;
      readStream_->read(reinterpret_cast<char*>(&value), sizeof(value));
      return value != 0;
    } else if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
      T value;
      readStream_->read(reinterpret_cast<char*>(&value), sizeof(value));
      return value;
    } else if constexpr (std::is_same_v<T, int16_t>) {
      return static_cast<int16_t>(readUInt16());
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      return readUInt16();
    } else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
      return static_cast<int32_t>(readUInt32());
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      return readUInt32();
    } else if constexpr (std::is_same_v<T, int64_t>) {
      return static_cast<int64_t>(readUInt64());
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      return readUInt64();
    } else if constexpr (std::is_same_v<T, float>) {
      return readFloat();
    } else if constexpr (std::is_same_v<T, double>) {
      return readDouble();
    } else {
      static_assert(!std::is_same_v<T, T>, "Unsupported type for getValue");
    }
  }

  template <typename T>
  void setValue(NodeType node, const T& value) {
    if (!writeStream_) return;  // Only write during serialization

    if constexpr (std::is_same_v<T, std::string>) {
      writeStringToStream(value);
    } else if constexpr (std::is_same_v<T, bool>) {
      uint8_t byteVal = value ? 1 : 0;
      writeStream_->write(reinterpret_cast<const char*>(&byteVal), 1);
    } else if constexpr (std::is_same_v<T, int8_t> || std::is_same_v<T, uint8_t>) {
      writeStream_->write(reinterpret_cast<const char*>(&value), 1);
    } else if constexpr (std::is_same_v<T, int16_t>) {
      writeUInt16ToStream(static_cast<uint16_t>(value));
    } else if constexpr (std::is_same_v<T, uint16_t>) {
      writeUInt16ToStream(value);
    } else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, int>) {
      writeUInt32ToStream(static_cast<uint32_t>(value));
    } else if constexpr (std::is_same_v<T, uint32_t>) {
      writeUInt32ToStream(value);
    } else if constexpr (std::is_same_v<T, int64_t>) {
      writeUInt64ToStream(static_cast<uint64_t>(value));
    } else if constexpr (std::is_same_v<T, uint64_t>) {
      writeUInt64ToStream(value);
    } else if constexpr (std::is_same_v<T, float>) {
      writeFloatToStream(value);
    } else if constexpr (std::is_same_v<T, double>) {
      writeDoubleToStream(value);
    } else {
      static_assert(!std::is_same_v<T, T>, "Unsupported type for setValue");
    }
  }

 private:
  // Direct stream writing functions
  void writeStringToStream(const std::string& value) {
    writeUInt32ToStream(static_cast<uint32_t>(value.length()));
    writeStream_->write(value.data(), value.length());
  }

  void writeUInt16ToStream(uint16_t value) {
    uint16_t converted = htole16(value);
    writeStream_->write(reinterpret_cast<const char*>(&converted), sizeof(uint16_t));
  }

  void writeUInt32ToStream(uint32_t value) {
    uint32_t converted = htole32(value);
    writeStream_->write(reinterpret_cast<const char*>(&converted), sizeof(uint32_t));
  }

  void writeUInt64ToStream(uint64_t value) {
    uint64_t converted = htole64(value);
    writeStream_->write(reinterpret_cast<const char*>(&converted), sizeof(uint64_t));
  }

  void writeFloatToStream(float value) {
    uint32_t intBits;
    std::memcpy(&intBits, &value, sizeof(float));
    writeUInt32ToStream(intBits);
  }

  void writeDoubleToStream(double value) {
    uint64_t longBits;
    std::memcpy(&longBits, &value, sizeof(double));
    writeUInt64ToStream(longBits);
  }

  // Read functions for deserialization
  std::string readString() {
    uint32_t length = readUInt32();
    std::string result(length, '\0');
    if (length > 0) {
      readStream_->read(&result[0], length);
    }
    return result;
  }

  uint16_t readUInt16() {
    uint16_t value;
    readStream_->read(reinterpret_cast<char*>(&value), sizeof(value));
    return le16toh(value);
  }

  uint32_t readUInt32() {
    uint32_t value;
    readStream_->read(reinterpret_cast<char*>(&value), sizeof(value));
    return le32toh(value);
  }

  uint64_t readUInt64() {
    uint64_t value;
    readStream_->read(reinterpret_cast<char*>(&value), sizeof(value));
    return le64toh(value);
  }

  float readFloat() {
    uint32_t intBits = readUInt32();
    float result;
    std::memcpy(&result, &intBits, sizeof(float));
    return result;
  }

  double readDouble() {
    uint64_t longBits = readUInt64();
    double result;
    std::memcpy(&result, &longBits, sizeof(double));
    return result;
  }
};

// Convenience alias for binary serializable types
template <typename T>
using BinarySerializable = Serializable<T, BinaryContext>;

}  // namespace serialization

// Global alias for easier access
template <typename T>
using BinarySerializable = serialization::BinarySerializable<T>;

}  // namespace lazy