# Serialization tests
add_executable(serialization-tests test_serializable.cpp test_json.cpp
                                   test_text.cpp test_binary.cpp test_yaml.cpp)

target_link_libraries(serialization-tests PRIVATE lazy-cpp GTest::gtest_main
                                                  ${RapidJSON_LIBRARIES})

# fkYAML is header-only and included via lazy-cpp target - no linking needed

target_include_directories(
  serialization-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                              ${RapidJSON_INCLUDE_DIRS})

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(serialization-tests)
