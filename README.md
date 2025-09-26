# lazy-cpp

A C++ library for automatic serialization with pluggable backends.

## Quick Start

```cpp
#include "lazy/serialization/text.h"
#include <iostream>

class Person : public lazy::TextSerializable<Person> {
public:
  SERIALIZABLE_FIELD(std::string, name, "John");
  SERIALIZABLE_FIELD(int, age, 25);
};

int main() {
  Person person;
  person.name = "Alice";
  person.age = 30;
  
  person.serialize(std::cout);  // Outputs: name = "Alice", age = 30
  return 0;
}
```

## Features

- **Text serialization**: Human-readable key-value format (no dependencies)
- **Binary serialization**: Compact, endian-safe binary format (no dependencies)  
- **JSON serialization**: JSON format via RapidJSON (optional component)
- **YAML serialization**: YAML format via fkYAML (optional component)
- **Header-only**: Easy to integrate
- **Modern CMake**: Component-based package system

## Building

### Basic build (text + binary only):
```bash
mkdir build && cd build
cmake -DLAZY_BUILD_TESTS=OFF ..  # Disable tests for minimal build
make
```

### Development build (with all features and tests):
```bash  
mkdir build && cd build
cmake ..  # Tests auto-enable JSON/YAML components
make
```

### With optional components:
```bash
cmake -DLAZY_SERIALIZATION_JSON=ON -DLAZY_SERIALIZATION_YAML=ON ..
make
```

### Using in your project:
```cmake
find_package(lazy-cpp REQUIRED COMPONENTS serialization)
target_link_libraries(your_target lazy-cpp::serialization)

# Or for JSON support:
find_package(lazy-cpp REQUIRED COMPONENTS serialization-json)
target_link_libraries(your_target lazy-cpp::serialization-json)
```

See [docs/BUILD_SYSTEM.md](docs/BUILD_SYSTEM.md) for detailed build and packaging information.
