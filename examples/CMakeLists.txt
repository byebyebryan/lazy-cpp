# Examples CMakeLists.txt

# Create examples for available serialization formats
set(EXAMPLE_TARGETS)

# Text format example (always available - core functionality)
add_executable(examples-text serializable_examples.cpp)
target_compile_definitions(examples-text PRIVATE USE_TEXT)
target_link_libraries(examples-text lazy-cpp::serialization)
list(APPEND EXAMPLE_TARGETS examples-text)

# Binary format example (always available - core functionality)
add_executable(examples-binary serializable_examples.cpp)
target_compile_definitions(examples-binary PRIVATE USE_BINARY)
target_link_libraries(examples-binary lazy-cpp::serialization)
list(APPEND EXAMPLE_TARGETS examples-binary)

# JSON format example (always available - uses lazy JSON by default)
add_executable(examples-json serializable_examples.cpp)
target_compile_definitions(examples-json PRIVATE USE_JSON)
target_link_libraries(examples-json lazy-cpp::serialization)
list(APPEND EXAMPLE_TARGETS examples-json)

# RapidJSON format example (only if RapidJSON support is enabled)
if(LAZY_SERIALIZATION_RAPID_JSON)
  add_executable(examples-rapidjson serializable_examples.cpp)
  target_compile_definitions(examples-rapidjson PRIVATE USE_JSON)
  target_link_libraries(examples-rapidjson lazy-cpp::serialization-rapid-json)
  list(APPEND EXAMPLE_TARGETS examples-rapidjson)
endif()

# YAML format example (only if YAML support is enabled)
if(LAZY_SERIALIZATION_YAML)
  add_executable(examples-yaml serializable_examples.cpp)
  target_compile_definitions(examples-yaml PRIVATE USE_YAML)
  target_link_libraries(examples-yaml lazy-cpp::serialization-yaml)
  list(APPEND EXAMPLE_TARGETS examples-yaml)
endif()

# Keep the original executable for backward compatibility (defaults to text)
add_executable(serializable-examples serializable_examples.cpp)
target_link_libraries(serializable-examples lazy-cpp::serialization)
list(APPEND EXAMPLE_TARGETS serializable-examples)

# Set output directory for all examples
if(EXAMPLE_TARGETS)
  set_target_properties(
    ${EXAMPLE_TARGETS} PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                  ${CMAKE_BINARY_DIR}/examples)
endif()
