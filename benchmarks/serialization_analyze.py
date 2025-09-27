#!/usr/bin/env python3
"""
Process Google Benchmark JSON output and create organized, readable reports.
"""
import json
import sys
from collections import defaultdict
from typing import Dict, List, Any

def parse_benchmark_name(name: str) -> Dict[str, str]:
    """Parse benchmark name into components."""
    parts = name.split('_')
    if len(parts) < 3:
        return {"category": "unknown", "adapter": "unknown", "data": "unknown", "operation": "unknown"}
    
    if parts[1] == "Serializable":
        return {
            "category": "Serializable",
            "adapter": parts[2],
            "data": parts[3] if len(parts) > 3 else "unknown",
            "operation": parts[4] if len(parts) > 4 else "serialize"
        }
    elif parts[1] == "MultiSerializable":
        return {
            "category": "MultiSerializable", 
            "adapter": parts[2],
            "data": parts[3] if len(parts) > 3 else "unknown",
            "operation": parts[4] if len(parts) > 4 else "serialize"
        }
    elif parts[1] == "Adapter":
        return {
            "category": "Adapter",
            "adapter": parts[2],
            "data": parts[3] if len(parts) > 3 else "unknown", 
            "operation": parts[4] if len(parts) > 4 else "serialize"
        }
    
    return {"category": "unknown", "adapter": "unknown", "data": "unknown", "operation": "unknown"}

def format_time(ns: float) -> str:
    """Format time in appropriate units."""
    if ns < 1000:
        return f"{ns:.0f}ns"
    elif ns < 1000000:
        return f"{ns/1000:.1f}μs"
    else:
        return f"{ns/1000000:.1f}ms"

def calculate_overhead(baseline: float, comparison: float) -> str:
    """Calculate overhead percentage."""
    if baseline == 0:
        return "N/A"
    overhead = ((comparison - baseline) / baseline) * 100
    if overhead > 0:
        return f"+{overhead:.1f}%"
    else:
        return f"{overhead:.1f}%"

def print_header(title: str):
    """Print a formatted header."""
    print(f"\n{'='*90}")
    print(f"{title:^90}")
    print(f"{'='*90}")

def get_multi_overhead(benchmarks: List[Dict], adapter: str, data: str, operation: str) -> str:
    """Get MultiSerializable overhead for given parameters."""
    serializable_time = None
    multi_time = None
    
    for bench in benchmarks:
        name_parts = parse_benchmark_name(bench['name'])
        if (name_parts['adapter'] == adapter and 
            name_parts['data'] == data and 
            name_parts['operation'] == operation):
            if name_parts['category'] == 'Serializable':
                serializable_time = bench['cpu_time']
            elif name_parts['category'] == 'MultiSerializable':
                multi_time = bench['cpu_time']
    
    if serializable_time and multi_time:
        return calculate_overhead(serializable_time, multi_time)
    return "N/A"

def print_adapter_comparison(benchmarks: List[Dict], data_type: str):
    """Print comprehensive adapter comparison for a data type."""
    print_header(f"🚀 {data_type.upper()} DATA PERFORMANCE")
    
    # Get all adapter results for this data type
    adapter_results = defaultdict(dict)
    
    for bench in benchmarks:
        name_parts = parse_benchmark_name(bench['name'])
        if name_parts['category'] == 'Adapter' and name_parts['data'].lower() == data_type.lower():
            adapter = name_parts['adapter']
            operation = name_parts['operation']
            adapter_results[adapter][operation] = bench['cpu_time']
    
    # Get overhead data for all adapters that have Serializable versions
    overhead_data = {}
    for adapter in ['Text', 'Binary', 'LazyJson', 'RapidJson', 'Yaml']:
        for operation in ['Serialize', 'Deserialize']:
            key = f"{adapter}_{operation}"
            overhead_data[key] = get_multi_overhead(benchmarks, adapter, data_type, operation)
    
    # Print table
    print(f"{'Adapter':<12} {'Serialize':<12} {'Deserialize':<12} {'Runtime vs Static (S)':<20} {'Runtime vs Static (D)':<20}")
    print("-" * 92)
    
    # Sort adapters for consistent ordering
    adapter_order = ['Text', 'Binary', 'LazyJson', 'RapidJson', 'Yaml']
    
    for adapter in adapter_order:
        if adapter in adapter_results:
            results = adapter_results[adapter]
            serialize_time = format_time(results.get('Serialize', 0)) if 'Serialize' in results else "N/A"
            deserialize_time = format_time(results.get('Deserialize', 0)) if 'Deserialize' in results else "N/A"
            
            # Get overhead if available
            serialize_overhead = overhead_data.get(f"{adapter}_Serialize", "N/A")
            deserialize_overhead = overhead_data.get(f"{adapter}_Deserialize", "N/A")
            
            print(f"{adapter:<12} {serialize_time:<12} {deserialize_time:<12} {serialize_overhead:<20} {deserialize_overhead:<20}")

def print_performance_rankings(benchmarks: List[Dict]):
    """Print overall performance ranking based on average performance."""
    print_header("🏆 OVERALL PERFORMANCE RANKING")
    
    # Calculate average performance for each adapter
    adapter_totals = defaultdict(list)
    
    for bench in benchmarks:
        name_parts = parse_benchmark_name(bench['name'])
        if name_parts['category'] == 'Adapter':
            adapter_totals[name_parts['adapter']].append(bench['cpu_time'])
    
    # Calculate averages and rank
    adapter_averages = []
    for adapter, times in adapter_totals.items():
        avg_time = sum(times) / len(times)
        adapter_averages.append({'adapter': adapter, 'avg_time': avg_time, 'count': len(times)})
    
    # Sort by average time (ascending = faster)
    adapter_averages.sort(key=lambda x: x['avg_time'])
    
    print(f"{'Rank':<6} {'Adapter':<12} {'Avg Time':<12} {'Benchmarks':<12} {'vs Middle':<12}")
    print("-" * 58)
    
    # Use middle performer as baseline
    middle_index = len(adapter_averages) // 2
    baseline_time = adapter_averages[middle_index]['avg_time']
    
    for i, result in enumerate(adapter_averages, 1):
        if i == middle_index + 1:
            vs_baseline = "baseline"
        else:
            vs_baseline = calculate_overhead(baseline_time, result['avg_time'])
        
        medal = "🥇" if i == 1 else "🥈" if i == 2 else "🥉" if i == 3 else f"{i}."
        print(f"{medal:<6} {result['adapter']:<12} {format_time(result['avg_time']):<12} "
              f"{result['count']}x{'':<7} {vs_baseline:<12}")

def print_scaling_analysis(benchmarks: List[Dict]):
    """Analyze how adapters scale from Simple to Complex data."""
    print_header("📈 COMPLEXITY SCALING ANALYSIS")
    
    # Get simple and complex times for each adapter
    scaling_data = defaultdict(dict)
    
    for bench in benchmarks:
        name_parts = parse_benchmark_name(bench['name'])
        if name_parts['category'] == 'Adapter' and name_parts['data'] in ['Simple', 'Complex']:
            key = name_parts['adapter']
            if name_parts['data'] not in scaling_data[key]:
                scaling_data[key][name_parts['data']] = []
            scaling_data[key][name_parts['data']].append(bench['cpu_time'])
    
    # Calculate averages and scaling
    print(f"{'Adapter':<12} {'Simple Avg':<12} {'Complex Avg':<12} {'Scaling':<12}")
    print("-" * 52)
    
    for adapter, data in scaling_data.items():
        if 'Simple' in data and 'Complex' in data:
            simple_avg = sum(data['Simple']) / len(data['Simple'])
            complex_avg = sum(data['Complex']) / len(data['Complex'])
            scaling = calculate_overhead(simple_avg, complex_avg)
            
            print(f"{adapter:<12} {format_time(simple_avg):<12} {format_time(complex_avg):<12} {scaling:<12}")


def main():
    """Main processing function."""
    if len(sys.argv) < 2:
        print("Usage: python3 analyze_serialization.py <benchmark_results.json>")
        sys.exit(1)
    
    try:
        with open(sys.argv[1], 'r') as f:
            data = json.load(f)
    except (FileNotFoundError, json.JSONDecodeError) as e:
        print(f"Error reading JSON file: {e}")
        sys.exit(1)
    
    benchmarks = data.get('benchmarks', [])
    if not benchmarks:
        print("No benchmark data found in JSON file")
        sys.exit(1)
    
    print("🚀 Lazy-CPP Serialization Benchmark Analysis")
    print(f"Generated from {len(benchmarks)} benchmark measurements")
    
    # Print organized results
    print_adapter_comparison(benchmarks, "Simple")
    print_adapter_comparison(benchmarks, "Complex")
    print_performance_rankings(benchmarks)
    print_scaling_analysis(benchmarks)
    
    print(f"\n{'='*90}")
    print("Analysis complete!")
    print(f"{'='*90}")

if __name__ == "__main__":
    main()