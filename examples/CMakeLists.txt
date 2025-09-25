# Examples CMakeLists.txt

# Create executable for serializable examples
add_executable(serializable-examples serializable_examples.cpp)
target_link_libraries(serializable-examples lazy-cpp)

# Set output directory for examples
set_target_properties(serializable-examples PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples
)
