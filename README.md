# lazy-cpp

[![MIT License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Header Only](https://img.shields.io/badge/header--only-yes-green.svg)]()

A header-only C++ library for developers who want powerful features without the hassle. 

Because life's too short for boilerplate code. ✨

## 🎯 Serialization

Serialize any class with a single macro per field - no boilerplate, no repetition!

```cpp
#include "lazy/serialization/json.h"
using namespace lazy::serialization;

class Config : public LazyJsonSerializable<Config> {
 public:
  SERIALIZABLE_FIELD(std::string, name, "MyApp");
  SERIALIZABLE_FIELD(int, port, 8080);
  SERIALIZABLE_FIELD(bool, debug, false);
};

Config config;
config.serialize(std::cout);  // That's it! 🎉
```

👉 **[Full serialization guide →](docs/serialization.md)**

## 📦 Installation

**Header-only** - just drop it in your project! 📁

```bash
git clone https://github.com/your-username/lazy-cpp.git
# Copy include/lazy/ to your project
```

Or use CMake:
```cmake
find_package(lazy-cpp REQUIRED)
target_link_libraries(your_target lazy-cpp::lazy-cpp)
```

## 🔨 Build Examples

```bash
./build.sh
./build/examples/serializable-examples
```

## 📋 Requirements

- C++17 or later
- That's it! No external dependencies for core functionality
- Optional: RapidJSON for RapidJSON support, fkYAML for YAML support

## 🚀 What's Next?

Serialization is just the beginning! More lazy modules coming soon... 🚀

- **lazy/validation** - Schema validation made simple
- **lazy/config** - Configuration management
- **lazy/logging** - Because `std::cout` gets old

## 🤝 Contributing

Found a bug? Want a feature? PRs welcome! 🤝

Just keep it simple, well-tested, and in the spirit of being lazy (in a good way).

## 📜 License

MIT License - see [LICENSE](LICENSE) for details.

Because sharing is caring, and lawyers are scary. 📜✨