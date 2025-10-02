#!/usr/bin/env python3
"""
Benchmark Visualization Tool for JazzyOrderBook

This script runs Google Benchmark tests, parses the JSON output,
and creates visualization plots comparing JazzyOrderBook vs MapOrderBook performance.
"""

import json
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Any
import argparse

try:
    import matplotlib.pyplot as plt
    import pandas as pd
    import numpy as np
except ImportError:
    print("Error: Required packages not installed.")
    print("Please run: pip install -r requirements.txt")
    sys.exit(1)


def run_benchmarks(benchmark_path: str, output_json: str) -> bool:
    """Run the benchmark executable and save results to JSON."""
    try:
        print(f"Running benchmarks from: {benchmark_path}")
        cmd = [benchmark_path, f"--benchmark_format=json", f"--benchmark_out={output_json}"]
        result = subprocess.run(cmd, capture_output=True, text=True, check=True)
        print("Benchmarks completed successfully!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running benchmarks: {e}")
        print(f"stderr: {e.stderr}")
        return False
    except FileNotFoundError:
        print(f"Error: Benchmark executable not found at {benchmark_path}")
        print("Please build the project first with: cmake --build build")
        return False


def parse_benchmark_results(json_path: str) -> pd.DataFrame:
    """Parse Google Benchmark JSON output into a pandas DataFrame."""
    with open(json_path, 'r') as f:
        data = json.load(f)

    benchmarks = data.get('benchmarks', [])

    records = []
    for bench in benchmarks:
        # Skip aggregate results (mean, median, stddev) - only process iteration results
        if bench.get('run_type') != 'iteration':
            continue

        name = bench['name']

        # Parse benchmark name: BM_<Implementation>_<Operation>/size
        parts = name.split('/')
        base_name = parts[0]
        size = int(parts[1]) if len(parts) > 1 else 0

        # Extract implementation and operation
        name_parts = base_name.split('_')
        if len(name_parts) >= 3:
            implementation = name_parts[1]  # JazzyOrderBook or MapOrderBook
            operation = '_'.join(name_parts[2:])  # AddOrders, UpdateOrders, etc.

            records.append({
                'implementation': implementation,
                'operation': operation,
                'size': size,
                'real_time': bench.get('real_time', 0),
                'cpu_time': bench.get('cpu_time', 0),
                'time_unit': bench.get('time_unit', 'ns'),
                'iterations': bench.get('iterations', 0)
            })

    return pd.DataFrame(records)


def create_comparison_plots(df: pd.DataFrame, output_dir: Path):
    """Create comparison plots for all benchmark operations."""
    operations = df['operation'].unique()

    # Set style
    plt.style.use('seaborn-v0_8-darkgrid')
    colors = {'JazzyOrderBook': '#2E86AB', 'MapOrderBook': '#A23B72'}

    for operation in operations:
        op_data = df[df['operation'] == operation]

        # Create figure with two subplots
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
        fig.suptitle(f'{operation} Performance Comparison', fontsize=16, fontweight='bold')

        # Plot 1: Time vs Size (log-log scale)
        for impl in op_data['implementation'].unique():
            impl_data = op_data[op_data['implementation'] == impl].sort_values('size')
            ax1.plot(impl_data['size'], impl_data['cpu_time'],
                    marker='o', linewidth=2, markersize=6,
                    label=impl, color=colors.get(impl, None))

        ax1.set_xlabel('Input Size (N)', fontsize=12)
        ax1.set_ylabel('Time (ns)', fontsize=12)
        ax1.set_title('Execution Time vs Input Size', fontsize=13)
        ax1.set_xscale('log', base=2)
        ax1.set_yscale('log')
        ax1.legend(fontsize=10)
        ax1.grid(True, alpha=0.3)

        # Plot 2: Speedup/Ratio
        jazzy_data = op_data[op_data['implementation'] == 'JazzyOrderBook'].sort_values('size')
        map_data = op_data[op_data['implementation'] == 'MapOrderBook'].sort_values('size')

        if len(jazzy_data) > 0 and len(map_data) > 0:
            # Merge on size to compare
            merged = pd.merge(jazzy_data, map_data, on='size', suffixes=('_jazzy', '_map'))
            merged['speedup'] = merged['cpu_time_map'] / merged['cpu_time_jazzy']

            ax2.plot(merged['size'], merged['speedup'],
                    marker='s', linewidth=2, markersize=6,
                    color='#F18F01', label='Speedup')
            ax2.axhline(y=1.0, color='red', linestyle='--', linewidth=1, alpha=0.7, label='Baseline (1x)')

            ax2.set_xlabel('Input Size (N)', fontsize=12)
            ax2.set_ylabel('Speedup (MapOrderBook / JazzyOrderBook)', fontsize=12)
            ax2.set_title('Performance Ratio', fontsize=13)
            ax2.set_xscale('log', base=2)
            ax2.legend(fontsize=10)
            ax2.grid(True, alpha=0.3)

            # Add text annotation for mean speedup
            mean_speedup = merged['speedup'].mean()
            ax2.text(0.05, 0.95, f'Mean Speedup: {mean_speedup:.2f}x',
                    transform=ax2.transAxes, fontsize=11,
                    verticalalignment='top',
                    bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

        plt.tight_layout()

        # Save figure
        output_path = output_dir / f'{operation}_comparison.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"Saved plot: {output_path}")
        plt.close()


def create_summary_plot(df: pd.DataFrame, output_dir: Path):
    """Create a summary plot showing all operations."""
    operations = df['operation'].unique()

    fig, axes = plt.subplots(2, 3, figsize=(18, 12))
    fig.suptitle('JazzyOrderBook vs MapOrderBook - Performance Summary',
                 fontsize=18, fontweight='bold')

    axes = axes.flatten()
    colors = {'JazzyOrderBook': '#2E86AB', 'MapOrderBook': '#A23B72'}

    for idx, operation in enumerate(operations):
        if idx >= len(axes):
            break

        ax = axes[idx]
        op_data = df[df['operation'] == operation]

        for impl in op_data['implementation'].unique():
            impl_data = op_data[op_data['implementation'] == impl].sort_values('size')
            ax.plot(impl_data['size'], impl_data['cpu_time'],
                   marker='o', linewidth=2, markersize=4,
                   label=impl, color=colors.get(impl, None))

        ax.set_xlabel('Input Size', fontsize=10)
        ax.set_ylabel('Time (ns)', fontsize=10)
        ax.set_title(operation, fontsize=12, fontweight='bold')
        ax.set_xscale('log', base=2)
        ax.set_yscale('log')
        ax.legend(fontsize=8)
        ax.grid(True, alpha=0.3)

    # Hide unused subplots
    for idx in range(len(operations), len(axes)):
        axes[idx].axis('off')

    plt.tight_layout()

    output_path = output_dir / 'summary_all_operations.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved summary plot: {output_path}")
    plt.close()


def create_bar_chart_comparison(df: pd.DataFrame, output_dir: Path):
    """Create bar charts comparing implementations at specific sizes."""
    # Get the largest size for comparison
    max_size = df['size'].max()
    large_size_data = df[df['size'] == max_size].copy()

    operations = large_size_data['operation'].unique()
    implementations = large_size_data['implementation'].unique()

    fig, ax = plt.subplots(figsize=(12, 8))

    x = np.arange(len(operations))
    width = 0.35

    colors = {'JazzyOrderBook': '#2E86AB', 'MapOrderBook': '#A23B72'}

    # Calculate actual operation counts for each benchmark type
    def get_operation_count(operation: str, size: int) -> int:
        """Return the actual number of operations performed in the benchmark."""
        if operation == 'AddOrders':
            return size  # N add operations
        elif operation == 'UpdateOrders':
            return size  # N update operations
        elif operation == 'DeleteOrders':
            return size  # N delete operations (plus N adds in setup, not timed)
        elif operation == 'VolumeLookup':
            return size * 2  # N ticks × 2 (bid + ask)
        elif operation == 'GetOrderAtLevel':
            return 40  # Fixed: 20 levels × 2 (bid + ask)
        elif operation == 'MixedOps':
            return size  # N mixed operations
        else:
            return size  # Default fallback

    for i, impl in enumerate(sorted(implementations)):
        impl_data = large_size_data[large_size_data['implementation'] == impl]
        times = []
        for op in operations:
            op_data = impl_data[impl_data['operation'] == op]
            if len(op_data) > 0:
                total_time = op_data['cpu_time'].values[0]
                size = op_data['size'].values[0]
                num_ops = get_operation_count(op, size)
                mean_time = total_time / num_ops
                times.append(mean_time)
            else:
                times.append(0)

        offset = width * (i - 0.5)
        bars = ax.bar(x + offset, times, width, label=impl, color=colors.get(impl, None))

        # Add value labels on bars
        for bar in bars:
            height = bar.get_height()
            ax.text(bar.get_x() + bar.get_width() / 2., height,
                   f'{height:.1f}',
                   ha='center', va='bottom', fontsize=8)

    # Market statistics from benchmarks/orderbook_benchmarks.cpp
    # market_statistics<int, 130, 90, 110, 2000>
    daily_high, daily_low, expected_range_bp = 130, 90, 2000
    num_buckets = int((daily_high - daily_low) * (1.0 + expected_range_bp / 10000.0))
    tick_range = daily_high - daily_low + 1

    ax.set_xlabel('Operation', fontsize=12, fontweight='bold')
    ax.set_ylabel('Mean Time per Operation (ns)', fontsize=12, fontweight='bold')
    title = f'Mean Performance per Operation\n(Book Size: {max_size} orders, {num_buckets} price levels, tick range: {daily_low}-{daily_high})'
    ax.set_title(title, fontsize=13, fontweight='bold')
    ax.set_xticks(x)
    ax.set_xticklabels(operations, rotation=45, ha='right')
    ax.legend(fontsize=10)
    ax.grid(True, alpha=0.3, axis='y')

    plt.tight_layout()

    output_path = output_dir / 'bar_comparison.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved bar chart: {output_path}")
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Visualize JazzyOrderBook benchmark results'
    )
    parser.add_argument(
        '--benchmark-path',
        default='./build/benchmarks/benchmarks',
        help='Path to benchmark executable (default: ./build/benchmarks/benchmarks)'
    )
    parser.add_argument(
        '--json-input',
        help='Use existing JSON file instead of running benchmarks'
    )
    parser.add_argument(
        '--output-dir',
        default='./benchmark_results',
        help='Directory for output plots (default: ./benchmark_results)'
    )
    parser.add_argument(
        '--no-run',
        action='store_true',
        help='Skip running benchmarks (requires --json-input)'
    )

    args = parser.parse_args()

    # Create output directory
    output_dir = Path(args.output_dir)
    output_dir.mkdir(exist_ok=True)

    json_path = args.json_input or str(output_dir / 'benchmark_results.json')

    # Run benchmarks unless --no-run is specified
    if not args.no_run:
        if not run_benchmarks(args.benchmark_path, json_path):
            sys.exit(1)
    elif not args.json_input:
        print("Error: --no-run requires --json-input")
        sys.exit(1)

    # Parse results
    print(f"\nParsing results from: {json_path}")
    df = parse_benchmark_results(json_path)

    if df.empty:
        print("Error: No benchmark data found!")
        sys.exit(1)

    print(f"Found {len(df)} benchmark results")
    print(f"Operations: {', '.join(df['operation'].unique())}")
    print(f"Implementations: {', '.join(df['implementation'].unique())}")

    # Create visualizations
    print("\nGenerating plots...")
    create_comparison_plots(df, output_dir)
    create_summary_plot(df, output_dir)
    create_bar_chart_comparison(df, output_dir)

    print(f"\nAll plots saved to: {output_dir}")
    print("Done!")


if __name__ == '__main__':
    main()
