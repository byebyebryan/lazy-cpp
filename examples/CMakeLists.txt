# Examples CMakeLists.txt

# Consolidated examples that demonstrate both Serializable and MultiSerializable
add_executable(serializable-examples serializable_examples.cpp)
target_compile_features(serializable-examples PRIVATE cxx_std_17)
target_include_directories(serializable-examples
                           PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Set output directory
set_target_properties(
  serializable-examples PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                   ${CMAKE_BINARY_DIR}/examples)
