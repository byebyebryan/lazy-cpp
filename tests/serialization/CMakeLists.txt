# Serialization tests
add_executable(serialization-tests test_serializable.cpp test_json.cpp)

target_link_libraries(serialization-tests PRIVATE lazy-cpp GTest::gtest_main
                                                  ${RapidJSON_LIBRARIES})

target_include_directories(
  serialization-tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
                              ${RapidJSON_INCLUDE_DIRS})

# Discover and add tests
include(GoogleTest)
gtest_discover_tests(serialization-tests)
