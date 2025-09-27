// ================================================================================================
// Lazy-CPP Serialization Performance Benchmarks
// ================================================================================================
// This file benchmarks different serialization adapters to compare:
// - Adapter Performance: Which adapter is fastest for different data types
// - Serializable vs MultiSerializable: Compile-time vs runtime adapter selection
// - Data Complexity: How adapters scale from simple to complex data structures

#include <benchmark/benchmark.h>

#include <iostream>
#include <sstream>

// Include all serialization adapters
#include <lazy/serialization/binary.h>
#include <lazy/serialization/json_lazy.h>
#include <lazy/serialization/multi_serializable.h>
#include <lazy/serialization/serializable.h>
#include <lazy/serialization/text.h>

#ifdef LAZY_SERIALIZATION_RAPID_JSON
#include <lazy/serialization/json_rapid.h>
#endif

#ifdef LAZY_SERIALIZATION_YAML_ENABLED
#include <lazy/serialization/yaml.h>
#endif

using namespace lazy::serialization;

// ================================================================================================
// TEST DATA STRUCTURES
// ================================================================================================

// Simple data for basic performance testing
class SimpleData : public MultiSerializable<SimpleData> {
 public:
  MULTI_SERIALIZABLE_FIELD(std::string, name, "BenchmarkTest");
  MULTI_SERIALIZABLE_FIELD(int, id, 12345);
  MULTI_SERIALIZABLE_FIELD(double, value, 3.14159);
  MULTI_SERIALIZABLE_FIELD(bool, active, true);
};

// Complex nested data for realistic scenarios
class ComplexData : public MultiSerializable<ComplexData> {
 public:
  MULTI_SERIALIZABLE_FIELD(std::string, description, "Complex benchmark data");
  MULTI_SERIALIZABLE_FIELD(std::vector<std::string>, tags);
  MULTI_SERIALIZABLE_FIELD(std::vector<int>, numbers);
  MULTI_SERIALIZABLE_FIELD(SimpleData, nested);

  void populateTestData() {
    description = "This is a complex data structure for realistic benchmarking scenarios";
    tags = {"performance", "benchmark", "serialization", "lazy-cpp", "adapter"};
    numbers = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89};
    nested.name = "Nested Test Object";
    nested.id = 999;
    nested.value = 2.71828;
    nested.active = false;
  }
};

// Serializable versions for overhead comparison
#define DEFINE_SERIALIZABLE_TYPES(AdapterName, AdapterClass)                                       \
  class Simple##AdapterName##Data : public AdapterClass##Serializable<Simple##AdapterName##Data> { \
   public:                                                                                         \
    SERIALIZABLE_FIELD(std::string, name, "BenchmarkTest");                                        \
    SERIALIZABLE_FIELD(int, id, 12345);                                                            \
    SERIALIZABLE_FIELD(double, value, 3.14159);                                                    \
    SERIALIZABLE_FIELD(bool, active, true);                                                        \
  };                                                                                               \
                                                                                                   \
  class Complex##AdapterName##Data                                                                 \
      : public AdapterClass##Serializable<Complex##AdapterName##Data> {                            \
   public:                                                                                         \
    SERIALIZABLE_FIELD(std::string, description, "Complex benchmark data");                        \
    SERIALIZABLE_FIELD(std::vector<std::string>, tags);                                            \
    SERIALIZABLE_FIELD(std::vector<int>, numbers);                                                 \
    SERIALIZABLE_FIELD(Simple##AdapterName##Data, nested);                                         \
                                                                                                   \
    void populateTestData() {                                                                      \
      description = "This is a complex data structure for realistic benchmarking scenarios";       \
      tags = {"performance", "benchmark", "serialization", "lazy-cpp", "adapter"};                 \
      numbers = {1, 2, 3, 5, 8, 13, 21, 34, 55, 89};                                               \
      nested.name = "Nested Test Object";                                                          \
      nested.id = 999;                                                                             \
      nested.value = 2.71828;                                                                      \
      nested.active = false;                                                                       \
    }                                                                                              \
  };

DEFINE_SERIALIZABLE_TYPES(Text, Text)
DEFINE_SERIALIZABLE_TYPES(Binary, Binary)
DEFINE_SERIALIZABLE_TYPES(LazyJson, LazyJson)

#ifdef LAZY_SERIALIZATION_RAPID_JSON
DEFINE_SERIALIZABLE_TYPES(RapidJson, RapidJson)
#endif

#ifdef LAZY_SERIALIZATION_YAML_ENABLED
DEFINE_SERIALIZABLE_TYPES(Yaml, Yaml)
#endif

// ================================================================================================
// BENCHMARK HELPER FUNCTIONS
// ================================================================================================

SimpleData createSimpleTestData() {
  SimpleData data;
  data.name = "Performance Test";
  data.id = 42;
  data.value = 1.618;
  data.active = true;
  return data;
}

ComplexData createComplexTestData() {
  ComplexData data;
  data.populateTestData();
  return data;
}

// ================================================================================================
// BENCHMARK MACROS (Reduces boilerplate significantly)
// ================================================================================================

// Macro for adapter comparison benchmarks (using MultiSerializable)
#define BENCHMARK_ADAPTER(AdapterName, AdapterClass)                                    \
  static void BM_Adapter_##AdapterName##_Simple_Serialize(benchmark::State& state) {    \
    auto data = createSimpleTestData();                                                 \
    for (auto _ : state) {                                                              \
      std::ostringstream oss;                                                           \
      data.serialize<AdapterClass>(oss);                                                \
      benchmark::DoNotOptimize(oss.str());                                              \
    }                                                                                   \
  }                                                                                     \
  BENCHMARK(BM_Adapter_##AdapterName##_Simple_Serialize);                               \
                                                                                        \
  static void BM_Adapter_##AdapterName##_Simple_Deserialize(benchmark::State& state) {  \
    auto data = createSimpleTestData();                                                 \
    std::ostringstream oss;                                                             \
    data.serialize<AdapterClass>(oss);                                                  \
    std::string serialized = oss.str();                                                 \
    for (auto _ : state) {                                                              \
      SimpleData result;                                                                \
      std::istringstream iss(serialized);                                               \
      result.deserialize<AdapterClass>(iss);                                            \
      benchmark::DoNotOptimize(result.name);                                            \
    }                                                                                   \
  }                                                                                     \
  BENCHMARK(BM_Adapter_##AdapterName##_Simple_Deserialize);                             \
                                                                                        \
  static void BM_Adapter_##AdapterName##_Complex_Serialize(benchmark::State& state) {   \
    auto data = createComplexTestData();                                                \
    for (auto _ : state) {                                                              \
      std::ostringstream oss;                                                           \
      data.serialize<AdapterClass>(oss);                                                \
      benchmark::DoNotOptimize(oss.str());                                              \
    }                                                                                   \
  }                                                                                     \
  BENCHMARK(BM_Adapter_##AdapterName##_Complex_Serialize);                              \
                                                                                        \
  static void BM_Adapter_##AdapterName##_Complex_Deserialize(benchmark::State& state) { \
    auto data = createComplexTestData();                                                \
    std::ostringstream oss;                                                             \
    data.serialize<AdapterClass>(oss);                                                  \
    std::string serialized = oss.str();                                                 \
    for (auto _ : state) {                                                              \
      ComplexData result;                                                               \
      std::istringstream iss(serialized);                                               \
      result.deserialize<AdapterClass>(iss);                                            \
      benchmark::DoNotOptimize(result.description);                                     \
    }                                                                                   \
  }                                                                                     \
  BENCHMARK(BM_Adapter_##AdapterName##_Complex_Deserialize);

// Macro for Serializable vs MultiSerializable comparisons
#define BENCHMARK_SERIALIZABLE_VS_MULTI(AdapterName, AdapterClass)                                \
  static void BM_Serializable_##AdapterName##_Simple_Serialize(benchmark::State& state) {         \
    Simple##AdapterName##Data serializable;                                                       \
    serializable.name = "Performance Test";                                                       \
    serializable.id = 42;                                                                         \
    serializable.value = 1.618;                                                                   \
    serializable.active = true;                                                                   \
    for (auto _ : state) {                                                                        \
      std::ostringstream oss;                                                                     \
      serializable.serialize(oss);                                                                \
      benchmark::DoNotOptimize(oss.str());                                                        \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_Serializable_##AdapterName##_Simple_Serialize);                                    \
                                                                                                  \
  static void BM_MultiSerializable_##AdapterName##_Simple_Serialize(benchmark::State& state) {    \
    auto data = createSimpleTestData();                                                           \
    for (auto _ : state) {                                                                        \
      std::ostringstream oss;                                                                     \
      data.serialize<AdapterClass>(oss);                                                          \
      benchmark::DoNotOptimize(oss.str());                                                        \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_MultiSerializable_##AdapterName##_Simple_Serialize);                               \
                                                                                                  \
  static void BM_Serializable_##AdapterName##_Simple_Deserialize(benchmark::State& state) {       \
    Simple##AdapterName##Data serializable;                                                       \
    serializable.name = "Performance Test";                                                       \
    serializable.id = 42;                                                                         \
    serializable.value = 1.618;                                                                   \
    serializable.active = true;                                                                   \
    std::ostringstream oss;                                                                       \
    serializable.serialize(oss);                                                                  \
    std::string serialized = oss.str();                                                           \
    for (auto _ : state) {                                                                        \
      Simple##AdapterName##Data result;                                                           \
      std::istringstream iss(serialized);                                                         \
      result.deserialize(iss);                                                                    \
      benchmark::DoNotOptimize(result.name);                                                      \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_Serializable_##AdapterName##_Simple_Deserialize);                                  \
                                                                                                  \
  static void BM_MultiSerializable_##AdapterName##_Simple_Deserialize(benchmark::State& state) {  \
    auto data = createSimpleTestData();                                                           \
    std::ostringstream oss;                                                                       \
    data.serialize<AdapterClass>(oss);                                                            \
    std::string serialized = oss.str();                                                           \
    for (auto _ : state) {                                                                        \
      SimpleData result;                                                                          \
      std::istringstream iss(serialized);                                                         \
      result.deserialize<AdapterClass>(iss);                                                      \
      benchmark::DoNotOptimize(result.name);                                                      \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_MultiSerializable_##AdapterName##_Simple_Deserialize);                             \
                                                                                                  \
  static void BM_Serializable_##AdapterName##_Complex_Serialize(benchmark::State& state) {        \
    Complex##AdapterName##Data serializable;                                                      \
    serializable.populateTestData();                                                              \
    for (auto _ : state) {                                                                        \
      std::ostringstream oss;                                                                     \
      serializable.serialize(oss);                                                                \
      benchmark::DoNotOptimize(oss.str());                                                        \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_Serializable_##AdapterName##_Complex_Serialize);                                   \
                                                                                                  \
  static void BM_MultiSerializable_##AdapterName##_Complex_Serialize(benchmark::State& state) {   \
    auto data = createComplexTestData();                                                          \
    for (auto _ : state) {                                                                        \
      std::ostringstream oss;                                                                     \
      data.serialize<AdapterClass>(oss);                                                          \
      benchmark::DoNotOptimize(oss.str());                                                        \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_MultiSerializable_##AdapterName##_Complex_Serialize);                              \
                                                                                                  \
  static void BM_Serializable_##AdapterName##_Complex_Deserialize(benchmark::State& state) {      \
    Complex##AdapterName##Data serializable;                                                      \
    serializable.populateTestData();                                                              \
    std::ostringstream oss;                                                                       \
    serializable.serialize(oss);                                                                  \
    std::string serialized = oss.str();                                                           \
    for (auto _ : state) {                                                                        \
      Complex##AdapterName##Data result;                                                          \
      std::istringstream iss(serialized);                                                         \
      result.deserialize(iss);                                                                    \
      benchmark::DoNotOptimize(result.description);                                               \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_Serializable_##AdapterName##_Complex_Deserialize);                                 \
                                                                                                  \
  static void BM_MultiSerializable_##AdapterName##_Complex_Deserialize(benchmark::State& state) { \
    auto data = createComplexTestData();                                                          \
    std::ostringstream oss;                                                                       \
    data.serialize<AdapterClass>(oss);                                                            \
    std::string serialized = oss.str();                                                           \
    for (auto _ : state) {                                                                        \
      ComplexData result;                                                                         \
      std::istringstream iss(serialized);                                                         \
      result.deserialize<AdapterClass>(iss);                                                      \
      benchmark::DoNotOptimize(result.description);                                               \
    }                                                                                             \
  }                                                                                               \
  BENCHMARK(BM_MultiSerializable_##AdapterName##_Complex_Deserialize);

// ================================================================================================
// BENCHMARK REGISTRATIONS
// ================================================================================================

// Register all adapter benchmarks
BENCHMARK_ADAPTER(Text, TextAdapter)
BENCHMARK_ADAPTER(Binary, BinaryAdapter)
BENCHMARK_ADAPTER(LazyJson, LazyJsonAdapter)

#ifdef LAZY_SERIALIZATION_RAPID_JSON
BENCHMARK_ADAPTER(RapidJson, RapidJsonAdapter)
#endif

#ifdef LAZY_SERIALIZATION_YAML_ENABLED
BENCHMARK_ADAPTER(Yaml, YamlAdapter)
#endif

// Register all Serializable vs MultiSerializable benchmarks
BENCHMARK_SERIALIZABLE_VS_MULTI(Text, TextAdapter)
BENCHMARK_SERIALIZABLE_VS_MULTI(Binary, BinaryAdapter)
BENCHMARK_SERIALIZABLE_VS_MULTI(LazyJson, LazyJsonAdapter)

#ifdef LAZY_SERIALIZATION_RAPID_JSON
BENCHMARK_SERIALIZABLE_VS_MULTI(RapidJson, RapidJsonAdapter)
#endif

#ifdef LAZY_SERIALIZATION_YAML_ENABLED
BENCHMARK_SERIALIZABLE_VS_MULTI(Yaml, YamlAdapter)
#endif

// ================================================================================================
// MAIN FUNCTION
// ================================================================================================

int main(int argc, char** argv) {
  std::cout << "\n🚀 Lazy-CPP Serialization Benchmarks" << std::endl;
  std::cout << "=====================================" << std::endl;

  std::cout << "\n📊 What we're measuring:" << std::endl;
  std::cout << "  🔹 Serializable vs MultiSerializable: Compile-time vs runtime adapter selection "
               "performance"
            << std::endl;
  std::cout << "  🔹 Adapter Performance: Comparing TextAdapter, BinaryAdapter, LazyJsonAdapter, "
               "RapidJsonAdapter, YamlAdapter"
            << std::endl;
  std::cout << "  🔹 Data Complexity: Simple objects vs complex nested structures" << std::endl;
  std::cout << "  🔹 Operations: Both serialization and deserialization" << std::endl;

  std::cout << "\n💡 Reading the results:" << std::endl;
  std::cout << "  • Lower Time/Op = Better Performance" << std::endl;
  std::cout << "  • CPU time is per-operation average across many iterations" << std::endl;
  std::cout << "  • Google Benchmark automatically handles timing and statistics" << std::endl;
  std::cout << "  • Negative values in 'Multi vs Ser.' mean MultiSerializable is FASTER"
            << std::endl;

  std::cout << "\n⏱️  Running benchmarks..." << std::endl;
  std::cout << "==================================================" << std::endl;

  // Initialize and run Google Benchmark
  ::benchmark::Initialize(&argc, argv);
  if (::benchmark::ReportUnrecognizedArguments(argc, argv)) return 1;

  ::benchmark::RunSpecifiedBenchmarks();
  ::benchmark::Shutdown();

  std::cout << "\n✅ Benchmark completed!" << std::endl;

  std::cout << "\n📝 Benchmark categories explained:" << std::endl;
  std::cout
      << "  • BM_Serializable_*: Uses compile-time adapter selection (Serializable<T, Adapter>)"
      << std::endl;
  std::cout << "  • BM_MultiSerializable_*: Uses runtime adapter selection (MultiSerializable<T>)"
            << std::endl;
  std::cout << "  • BM_Adapter_*: Compares different adapters using MultiSerializable interface"
            << std::endl;

  return 0;
}
