cmake_minimum_required(VERSION 3.15)

project(
  lazy-cpp
  VERSION 1.0.0
  LANGUAGES CXX)

# Options for optional components
option(LAZY_SERIALIZATION_JSON "Enable JSON serialization support" OFF)
option(LAZY_SERIALIZATION_YAML "Enable YAML serialization support" OFF)
option(LAZY_BUILD_EXAMPLES "Build examples" ON)
option(LAZY_BUILD_TESTS "Build tests" ON)

# Auto-enable JSON/YAML when building tests (since tests exercise all
# functionality)
if(LAZY_BUILD_TESTS)
  if(NOT LAZY_SERIALIZATION_JSON)
    message(STATUS "Auto-enabling JSON support for tests")
    set(LAZY_SERIALIZATION_JSON
        ON
        CACHE BOOL "Enable JSON serialization support" FORCE)
  endif()
  if(NOT LAZY_SERIALIZATION_YAML)
    message(STATUS "Auto-enabling YAML support for tests")
    set(LAZY_SERIALIZATION_YAML
        ON
        CACHE BOOL "Enable YAML serialization support" FORCE)
  endif()
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core serialization library (text + binary support)
add_library(lazy-cpp-serialization INTERFACE)
add_library(lazy-cpp::serialization ALIAS lazy-cpp-serialization)

target_include_directories(
  lazy-cpp-serialization
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>)

target_compile_features(lazy-cpp-serialization INTERFACE cxx_std_17)

# Optional JSON support
if(LAZY_SERIALIZATION_JSON)
  # Try to find RapidJSON via pkg-config first
  find_package(PkgConfig QUIET)
  if(PKG_CONFIG_FOUND)
    pkg_check_modules(RapidJSON QUIET RapidJSON)
  endif()

  # If not found, use FetchContent
  if(NOT RapidJSON_FOUND)
    include(FetchContent)

    FetchContent_Declare(
      rapidjson
      GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
      GIT_TAG v1.1.0 # Use stable version instead of master
    )

    FetchContent_MakeAvailable(rapidjson)
    set(RapidJSON_INCLUDE_DIRS ${rapidjson_SOURCE_DIR}/include)
  endif()

  # Create JSON component target
  add_library(lazy-cpp-serialization-json INTERFACE)
  add_library(lazy-cpp::serialization-json ALIAS lazy-cpp-serialization-json)

  target_link_libraries(lazy-cpp-serialization-json
                        INTERFACE lazy-cpp::serialization)
  target_include_directories(lazy-cpp-serialization-json
                             INTERFACE ${RapidJSON_INCLUDE_DIRS})
  target_compile_definitions(lazy-cpp-serialization-json
                             INTERFACE LAZY_SERIALIZATION_JSON_ENABLED)
endif()

# Optional YAML support
if(LAZY_SERIALIZATION_YAML)
  include(FetchContent)

  FetchContent_Declare(
    fkYAML
    GIT_REPOSITORY https://github.com/fktn-k/fkYAML.git
    GIT_TAG v0.4.2)

  FetchContent_MakeAvailable(fkYAML)

  # Create YAML component target
  add_library(lazy-cpp-serialization-yaml INTERFACE)
  add_library(lazy-cpp::serialization-yaml ALIAS lazy-cpp-serialization-yaml)

  target_link_libraries(lazy-cpp-serialization-yaml
                        INTERFACE lazy-cpp::serialization)
  target_include_directories(
    lazy-cpp-serialization-yaml
    INTERFACE $<BUILD_INTERFACE:${fkyaml_SOURCE_DIR}/include>)
  target_compile_definitions(lazy-cpp-serialization-yaml
                             INTERFACE LAZY_SERIALIZATION_YAML_ENABLED)
endif()

# Build examples if requested
if(LAZY_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# Build tests if requested
if(LAZY_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

# Installation and export
include(GNUInstallDirs)

# Collect all targets to export
set(LAZY_TARGETS lazy-cpp-serialization)

if(LAZY_SERIALIZATION_JSON)
  list(APPEND LAZY_TARGETS lazy-cpp-serialization-json)
endif()

if(LAZY_SERIALIZATION_YAML)
  list(APPEND LAZY_TARGETS lazy-cpp-serialization-yaml)
endif()

# Install targets
install(
  TARGETS ${LAZY_TARGETS}
  EXPORT lazy-cpp-targets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# Install headers
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Install export file
install(
  EXPORT lazy-cpp-targets
  FILE lazy-cpp-targets.cmake
  NAMESPACE lazy-cpp::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/lazy-cpp)

# Generate and install config files
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
