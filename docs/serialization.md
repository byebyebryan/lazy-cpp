# lazy-cpp Serialization

## The Problem 🤔

We've all been there - you need to make a class serializable for configs, save states, or API communication. 

Sure, you *could* write all that serialization code by hand... but who has time for that? We want to be **lazy** (the good kind)! 😴

And boost.serialization? Let's just say... it's *very* boost. 😅

Take **[nlohmann/json](https://github.com/nlohmann/json)** for example - it's pretty nice:
```cpp
class Data {
  std::string strValue;
  int intValue;
  ...

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(Data, strValue, intValue, ...)
};

// or
class Data {
  std::string strValue;
  int intValue;
  ...
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Data, strValue, intValue, ...)
```

**But wait, there's a catch!** 🪤 See how each field appears *twice*? Once in your class, then again in that macro?

With 20+ fields, you WILL forget to update the macro someday. Runtime explosion guaranteed. Goodbye weekend! 💥🏠

**The dream:** Something as elegant as Python's `@dataclass` or C#'s `[Serializable]`:
```python
@dataclass
class Data:
  stdValue: str
  intValue: int  # Write once, serialize everywhere! ✨
```

**The reality:** C++ has no reflection, so this is impossible... unless you want to dive into code generation like protobuf (we don't). 🙅‍♂️

## The Solution ✨

**Enter lazy-cpp serialization** 🎭
```cpp
#include "lazy/serialization/json.h"
using namespace lazy::serialization;

class Data : public LazyJsonSerializable<Data> {
 public:
  SERIALIZABLE_FIELD(std::string, strValue, "default");
  SERIALIZABLE_FIELD(int, intValue, 42);
  // Add as many fields as you want - no repetition! 🎉
};

Data data;
data.serialize(std::cout);    // Easy serialize
data.deserialize(std::cin);   // Easy deserialize
```

**Why this rocks:** 🎆
- ✨ **Single declaration** - fields appear only once!
- 🚀 **Zero-cost abstraction** - CRTP, no virtual functions
- 🎨 **Pick your poison** - JSON, YAML, Binary, Text, or roll your own
- ⚡ **Blazingly fast** - everything resolved at compile time
- 🛡️ **Type-safe** - catch serialization bugs at compile time, not at 3AM

## 🔥 Advanced Features

### Nested Types 🪆

Because real life is complicated:

```cpp
class Address : public LazyJsonSerializable<Address> {
 public:
  SERIALIZABLE_FIELD(std::string, street);
  SERIALIZABLE_FIELD(std::string, city);
};

class Person : public LazyJsonSerializable<Person> {
 public:
  SERIALIZABLE_FIELD(std::string, name);
  SERIALIZABLE_FIELD(Address, home);     // Nested serialization - just works! ✨
  SERIALIZABLE_FIELD(std::vector<Address>, addresses);  // Collections too!
};
```

### Sealed Types (External Classes) 🔒

Got a class you can't modify? No problem!

```cpp
// Some library class you can't touch
class ThirdPartyData {
 public:
  std::string name;
  int value;
};

// Make it serializable from the outside
namespace lazy::serialization {
SERIALIZABLE_TYPE(LazyJsonAdapter, ThirdPartyData, name, value)
}

// Now use it anywhere
class MyClass : public LazyJsonSerializable<MyClass> {
 public:
  SERIALIZABLE_FIELD(ThirdPartyData, external);  // Works like magic! 🪄
};
```

## Serializable vs MultiSerializable 🤔

**Serializable<T, Adapter>** locks you into one format at compile time. Fast and simple! ⚡

**MultiSerializable<T>** lets you pick the format at runtime. Perfect when you need flexibility (like supporting both JSON and YAML configs, or testing with different formats). 🎭

**The MultiSerializable approach:**
```cpp
#include "lazy/serialization/multi_serializable.h"
using namespace lazy::serialization;

class FlexibleData : public MultiSerializable<FlexibleData> {
 public:
  MULTI_SERIALIZABLE_FIELD(std::string, strValue, "flexible");
  MULTI_SERIALIZABLE_FIELD(int, intValue, 123);
};

FlexibleData data;

// Pick your format at runtime! 🎨
data.serialize<LazyJsonAdapter>(jsonStream);
data.serialize<YamlAdapter>(yamlStream);  
data.serialize<BinaryAdapter>(binStream);

// Same object, multiple formats - chef's kiss 👨‍🍳💋
```

**Trade-offs to consider:**
- ✅ **Runtime flexibility** - choose format on the fly
- ✅ **Same nested & sealed type support** as regular Serializable
- ⚠️ **Tiny runtime cost** - uses type erasure under the hood
- 📝 **Compile-time adapter list** - specify which adapters you want enabled

## 🎭 Serialization Backends

**Built-in flavors:** 🍦

| Format | Include | Serializable Type | Dependencies |
|--------|---------|-------------------|-------------|
| **Binary** | `binary.h` | `BinarySerializable<T>` | None! ✨ |
| **Text** | `text.h` | `TextSerializable<T>` | None! ✨ |
| **JSON (Lazy)** | `json_lazy.h` | `LazyJsonSerializable<T>` | None! ✨ |
| **JSON (RapidJSON)** | `json_rapid.h` | `RapidJsonSerializable<T>` | RapidJSON |
| **YAML** | `yaml.h` | `YamlSerializable<T>` | fkYAML |

### 🔌 Setting Up Optional Dependencies

**The lazy way (recommended):** Let CMake handle everything! 🪄

```cmake
find_package(lazy-cpp REQUIRED COMPONENTS 
  serialization                    # Always available (Text, Binary, LazyJSON)
  serialization-rapid-json         # Optional: RapidJSON support
  serialization-yaml              # Optional: YAML support
)

target_link_libraries(your_target 
  lazy-cpp::serialization
  lazy-cpp::serialization-rapid-json  # Only if you need RapidJSON
  lazy-cpp::serialization-yaml       # Only if you need YAML
)
```

**That's it!** CMake automatically fetches RapidJSON and fkYAML for you. No manual installation needed! ✨

**Then just include and use:**
```cpp
#include "lazy/serialization/json_rapid.h"  // RapidJsonAdapter
#include "lazy/serialization/yaml.h"        // YAMLAdapter

// or
#include "lazy/serialization/multi_serializable.h" // RapidJsonAdapter and YAMLAdapter enabled!
```

**Manual installation:** If you prefer to manage dependencies yourself, you can still install RapidJSON/fkYAML manually and just use `lazy-cpp::serialization`.

**Philosophy:** We're not trying to be everything to everyone. But we make it stupid easy to add your own! 🛠️

### Roll Your Own Adapter 🎲

```cpp
// Your custom format adapter
class MyCustomAdapter {
 public:
  using NodeType = MyCustomNode;  // Your node type
  
  // The bare minimum - implement these 4 methods:
  static NodeType createNode();
  static void serialize(std::ostream& out, const NodeType& node);
  static NodeType deserialize(std::istream& in);
  static void setField(NodeType& node, const std::string& key, const NodeType& value);
  
  // Add getField, hasField for full read support...
};

// Use it!
class MyData : public Serializable<MyData, MyCustomAdapter> {
  SERIALIZABLE_FIELD(std::string, data);
};
```

**Want MultiSerializable support too?**
```cpp
// Just add your adapter to the list at compile time
namespace lazy::serialization {
template<>
struct AdapterList<MyData> {
  using type = std::tuple<LazyJsonAdapter, MyCustomAdapter>;
};
}
```

That's it! Your custom format now works everywhere. 🎆

## 🏆 Best Practices

**Field Defaults:** Always provide sensible defaults - they're lifesavers during versioning:
```cpp
SERIALIZABLE_FIELD(std::string, version, "1.0");  // ✅ Good
SERIALIZABLE_FIELD(std::string, version);         // ❌ Risky
```

**Versioning:** Add a version field to your root objects:
```cpp
class Config : public LazyJsonSerializable<Config> {
  SERIALIZABLE_FIELD(std::string, schema_version, "2.1");  // Future you will thank you
  // ... other fields
};
```

**Nested Collections:** They just work™, but be mindful of depth:
```cpp
SERIALIZABLE_FIELD(std::vector<std::vector<NestedClass>>, nested_matrix);  // Valid but... why? 😅
```

## 🔧 Troubleshooting

**"Field not found" during deserialization?**
- Check field names match exactly (case-sensitive)
- Make sure you provided default values
- Verify the input format is valid

**Compilation errors with custom types?**
- Ensure your type has a default constructor
- For sealed types, check the `SERIALIZABLE_TYPE` macro placement

**Performance concerns?**
- Use `Serializable<T, Adapter>` for compile-time format selection
- Binary format is fastest for pure speed
- Text format is great for debugging

## ⚡ Performance Notes

| Adapter | Speed | Size | Human Readable |
|---------|-------|------|----------------|
| Binary | 🚀🚀🚀 | 🤏 | ❌ |
| JSON (Lazy) | 🚀🚀 | 📏 | ✅ |
| JSON (Rapid) | 🚀🚀🚀 | 📏 | ✅ |
| YAML | 🚀 | 📏📏 | ✅ |
| Text | 🚀 | 📏📏📏 | ✅ |

Remember: premature optimization is the root of all evil. Start with what's readable, optimize if needed! 🌱
