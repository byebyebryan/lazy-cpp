# Examples CMakeLists.txt

# Create examples for each serialization format

# Text format example (default)
add_executable(examples-text serializable_examples.cpp)
target_compile_definitions(examples-text PRIVATE USE_TEXT)
target_link_libraries(examples-text lazy-cpp)

# JSON format example
add_executable(examples-json serializable_examples.cpp)
target_compile_definitions(examples-json PRIVATE USE_JSON)
target_link_libraries(examples-json lazy-cpp)

# Binary format example
add_executable(examples-binary serializable_examples.cpp)
target_compile_definitions(examples-binary PRIVATE USE_BINARY)
target_link_libraries(examples-binary lazy-cpp)

# YAML format example (needs yaml-cpp)
add_executable(examples-yaml serializable_examples.cpp)
target_compile_definitions(examples-yaml PRIVATE USE_YAML)
target_link_libraries(examples-yaml lazy-cpp)
if(TARGET yaml-cpp)
  target_link_libraries(examples-yaml yaml-cpp)
elseif(yaml-cpp_LIBRARIES)
  target_link_libraries(examples-yaml ${yaml-cpp_LIBRARIES})
endif()

# Keep the original executable for backward compatibility (defaults to text)
add_executable(serializable-examples serializable_examples.cpp)
target_link_libraries(serializable-examples lazy-cpp)

# Set output directory for all examples
set_target_properties(
  examples-text examples-json examples-binary examples-yaml
  serializable-examples PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   ${CMAKE_BINARY_DIR}/examples)
