cmake_minimum_required(VERSION 3.15)

project(
  lazy-cpp
  VERSION 1.0.0
  LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IntelliSense
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add include directories
include_directories(include)

# Find RapidJSON
find_package(PkgConfig REQUIRED)
pkg_check_modules(RapidJSON REQUIRED RapidJSON)

# If RapidJSON isn't found, try using FetchContent
if(NOT RapidJSON_FOUND)
  include(FetchContent)

  FetchContent_Declare(
    rapidjson
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG master)

  FetchContent_MakeAvailable(rapidjson)

  # RapidJSON is header-only, so we just need the include directories
  set(RapidJSON_INCLUDE_DIRS ${rapidjson_SOURCE_DIR}/include)
endif()

# Find yaml-cpp (optional dependency for YAML serialization support)
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
  pkg_check_modules(yaml-cpp QUIET yaml-cpp)
endif()

# If yaml-cpp isn't found, fetch it for tests and examples
if(NOT yaml-cpp_FOUND)
  include(FetchContent)

  FetchContent_Declare(
    yaml-cpp
    GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
    GIT_TAG master)

  FetchContent_MakeAvailable(yaml-cpp)

  # yaml-cpp target is now available
  set(yaml-cpp_FOUND TRUE)
endif()

# Create interface library for lazy-cpp headers
add_library(lazy-cpp INTERFACE)
target_include_directories(
  lazy-cpp INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                     $<INSTALL_INTERFACE:include> ${RapidJSON_INCLUDE_DIRS})
target_compile_features(lazy-cpp INTERFACE cxx_std_17)

# Note: yaml-cpp is an optional dependency. Consuming targets that use YAML
# serialization should link to yaml-cpp directly when needed.

# Add examples subdirectory
add_subdirectory(examples)

# Add tests subdirectory
enable_testing()
add_subdirectory(tests)

# Installation rules
include(GNUInstallDirs)

install(
  TARGETS lazy-cpp
  EXPORT lazy-cpp-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(
  EXPORT lazy-cpp-targets
  FILE lazy-cpp-targets.cmake
  NAMESPACE lazy::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lazy-cpp)

# Generate and install config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/lazy-cpp-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lazy-cpp-config.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lazy-cpp)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/lazy-cpp-config-version.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/lazy-cpp-config.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/lazy-cpp-config-version.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lazy-cpp)
