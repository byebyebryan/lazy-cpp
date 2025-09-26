# Serialization tests

# Base test sources (always available)
set(TEST_SOURCES test_serializable.cpp test_text.cpp test_binary.cpp)

# Add optional test sources based on enabled components
if(LAZY_SERIALIZATION_JSON)
  list(APPEND TEST_SOURCES test_json.cpp)
endif()

if(LAZY_SERIALIZATION_YAML)
  list(APPEND TEST_SOURCES test_yaml.cpp)
endif()

# Create test executable
add_executable(serialization-tests ${TEST_SOURCES})

# Base linking (always link core serialization and GTest)
target_link_libraries(serialization-tests PRIVATE lazy-cpp::serialization
                                                  GTest::gtest_main)

# Add optional component links
if(LAZY_SERIALIZATION_JSON)
  target_link_libraries(serialization-tests
                        PRIVATE lazy-cpp::serialization-json)
endif()

if(LAZY_SERIALIZATION_YAML)
  target_link_libraries(serialization-tests
                        PRIVATE lazy-cpp::serialization-yaml)
endif()

# Include directories
target_include_directories(serialization-tests
                           PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(serialization-tests)
