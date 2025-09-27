#!/bin/bash

# lazy-cpp build script
# Usage: ./build.sh [options]

set -e  # Exit on any error

# Default values
BUILD_TYPE="Release"
CLEAN=false
FORMAT=false
RUN_EXAMPLES=false
RUN_TESTS=false
RUN_BENCHMARKS=false
VERBOSE=false
BUILD_DIR="build"
GENERATOR="Ninja"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Print colored output
print_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
print_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
print_warning() { echo -e "${YELLOW}[WARNING]${NC} $1"; }
print_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# Function to show help
show_help() {
    cat << EOF
lazy-cpp build script

Usage: $0 [OPTIONS]

OPTIONS:
    -h, --help          Show this help message
    -b, --benchmark     Build and run benchmarks (with organized results if Python available)
    -c, --clean         Clean build directory before building
    -d, --debug         Build in Debug mode (default: Release)
    -f, --format        Run clang-format on all source files
    -g, --generator <G> Use the specified CMake generator (e.g., "Ninja")
    -r, --run           Run examples after building
    -t, --test          Run tests after building
    -v, --verbose       Verbose output

EXAMPLES:
    $0                  # Build in Release mode
    $0 -c -d            # Clean build in Debug mode
    $0 -f               # Format code only
    $0 -c -r            # Clean build and run examples
    $0 -t               # Build and run tests
    $0 -b               # Build and run benchmarks
    $0 -c -t -r -b      # Clean build, run tests, examples and benchmarks
    $0 --format --clean --debug --test --run --benchmark  # Full development cycle

EOF
}

# Function to format code
format_code() {
    print_info "Running clang-format on all source files..."

    if ! command -v clang-format &> /dev/null; then
        print_error "clang-format is not installed. Please install it first."
        return 1
    fi

    # Find all source files
    SOURCE_FILES=(
        $(find include -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" 2>/dev/null || true)
        $(find examples -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" 2>/dev/null || true)
        $(find tests -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" 2>/dev/null || true)
        $(find benchmarks -name "*.h" -o -name "*.hpp" -o -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" 2>/dev/null || true)
    )

    if [ ${#SOURCE_FILES[@]} -eq 0 ]; then
        print_warning "No source files found to format"
        return 0
    fi

    for file in "${SOURCE_FILES[@]}"; do
        if [ -f "$file" ]; then
            print_info "Formatting: $file"
            clang-format -i "$file"
        fi
    done

    print_success "Code formatting completed!"
}

# Function to clean build directory
clean_build() {
    print_info "Cleaning build directory..."
    if [ -d "$BUILD_DIR" ]; then
        rm -rf "$BUILD_DIR"
        print_success "Build directory cleaned"
    else
        print_info "Build directory doesn't exist, nothing to clean"
    fi
}

# Function to configure and build
build_project() {
    print_info "Building lazy-cpp in $BUILD_TYPE mode..."
    
    # Create build directory
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    
    # Configure with CMake
    CMAKE_ARGS=""
    if [ "$VERBOSE" = true ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_VERBOSE_MAKEFILE=ON"
    fi
    
    # Enable benchmarks if requested
    if [ "$RUN_BENCHMARKS" = true ]; then
        CMAKE_ARGS="$CMAKE_ARGS -DLAZY_BUILD_BENCHMARKS=ON"
    fi
    
    print_info "Configuring with CMake..."
    cmake -G "$GENERATOR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" $CMAKE_ARGS ..
    
    # Build
    print_info "Compiling..."
    cmake --build . -- -j$(nproc)
    
    # Note: compile_commands.json stays in build/ for cleaner project structure
    
    cd ..
    print_success "Build completed successfully!"
}

# Function to run examples
run_examples() {
    print_info "Running examples..."

    if [ ! -f "$BUILD_DIR/examples/serialization-examples" ]; then
        print_error "Example executable not found. Build the project first."
        return 1
    fi

    print_info "Running serialization-examples:"
    echo "----------------------------------------"
    cd "$BUILD_DIR"
    ./examples/serialization-examples
    cd ..
    echo "----------------------------------------"
    print_success "Examples completed!"
}

# Function to run tests
run_tests() {
    print_info "Running tests..."

    if [ ! -f "$BUILD_DIR/tests/serialization/serialization-tests" ]; then
        print_error "Test executable not found. Build the project first."
        return 1
    fi

    print_info "Running serialization tests:"
    echo "----------------------------------------"
    cd "$BUILD_DIR"
    ./tests/serialization/serialization-tests
    cd ..
    echo "----------------------------------------"
    print_success "Tests completed!"
}

# Function to run benchmarks
run_benchmarks() {
    print_info "Running benchmarks..."

    if [ ! -f "$BUILD_DIR/benchmarks/serialization-benchmarks" ]; then
        print_error "Benchmark executable not found. Build the project with benchmarks enabled first."
        return 1
    fi

    print_info "Running serialization benchmarks:"
    echo "========================================"
    cd "$BUILD_DIR"
    
    # Check if Python is available for post-processing
    PYTHON_CMD=""
    if command -v python3 &> /dev/null; then
        PYTHON_CMD="python3"
    elif command -v python &> /dev/null; then
        PYTHON_CMD="python"
    fi
    
    if [ -n "$PYTHON_CMD" ] && [ -f "../benchmarks/serialization_analyze.py" ]; then
        print_info "Running benchmarks with JSON output for post-processing..."
        
        # Run benchmarks with JSON output
        ./benchmarks/serialization-benchmarks --benchmark_format=json --benchmark_out=benchmark_results.json
        
        print_info "Processing results for better readability..."
        $PYTHON_CMD ../benchmarks/serialization_analyze.py benchmark_results.json
    else
        print_warning "Python not found or post-processor missing. Running with standard output..."
        ./benchmarks/serialization-benchmarks
    fi
    
    cd ..
    echo "========================================"
    print_success "Benchmarks completed!"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -b|--benchmark)
            RUN_BENCHMARKS=true
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -f|--format)
            FORMAT=true
            shift
            ;;
        -g|--generator)
            GENERATOR="$2"
            shift 2
            ;;
        -r|--run)
            RUN_EXAMPLES=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_help
            exit 1
            ;;
    esac
done

# Main execution
print_info "Starting lazy-cpp build process..."

# Format code if requested
if [ "$FORMAT" = true ]; then
    format_code
fi

# If only formatting was requested, exit here
if [ "$FORMAT" = true ] && [ "$CLEAN" = false ] && [ "$RUN_EXAMPLES" = false ] && [ "$RUN_TESTS" = false ] && [ "$RUN_BENCHMARKS" = false ] && [ "$BUILD_TYPE" = "Release" ]; then
    print_success "Code formatting completed. Exiting."
    exit 0
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    clean_build
fi

# Build the project
build_project

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    run_tests
fi

# Run examples if requested
if [ "$RUN_EXAMPLES" = true ]; then
    run_examples
fi

# Run benchmarks if requested
if [ "$RUN_BENCHMARKS" = true ]; then
    run_benchmarks
fi

print_success "All tasks completed successfully!"
