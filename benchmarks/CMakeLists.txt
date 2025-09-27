# Benchmarks CMakeLists.txt

include(FetchContent)

# Fetch Google Benchmark
FetchContent_Declare(
  googlebenchmark
  GIT_REPOSITORY https://github.com/google/benchmark.git
  GIT_TAG v1.9.4 # Use latest stable version
)

# Configure benchmark options
set(BENCHMARK_ENABLE_TESTING
    OFF
    CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_INSTALL
    OFF
    CACHE BOOL "" FORCE)
set(BENCHMARK_ENABLE_GTEST_TESTS
    OFF
    CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googlebenchmark)

# Serialization adapter benchmarks
add_executable(serialization-benchmarks serialization.cpp)
target_compile_features(serialization-benchmarks PRIVATE cxx_std_17)
target_include_directories(serialization-benchmarks
                           PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Link with Google Benchmark and core serialization library
target_link_libraries(serialization-benchmarks PRIVATE benchmark::benchmark
                                                       lazy-cpp::serialization)

# Add optional adapter support if enabled
if(LAZY_SERIALIZATION_RAPID_JSON)
  target_link_libraries(serialization-benchmarks
                        PRIVATE lazy-cpp::serialization-rapid-json)
endif()

if(LAZY_SERIALIZATION_YAML)
  target_link_libraries(serialization-benchmarks
                        PRIVATE lazy-cpp::serialization-yaml)
endif()

# Set output directory
set_target_properties(
  serialization-benchmarks PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                                      ${CMAKE_BINARY_DIR}/benchmarks)
