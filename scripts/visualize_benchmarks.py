#!/usr/bin/env python3
"""
Benchmark Visualization Tool for JazzyOrderBook

This script runs Google Benchmark tests, parses the JSON output,
and creates visualization plots comparing JazzyOrderBook vs MapOrderBook performance.
"""

import argparse
import json
import math
import re
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, List, Optional

try:
    import matplotlib.pyplot as plt
    import pandas as pd
    import numpy as np
except ImportError:
    print("Error: Required packages not installed.")
    print("Please run: pip install -r requirements.txt")
    sys.exit(1)

CONSOLE_LINE_RE = re.compile(
    r"(?P<name>\S+)\s+(?P<real>[\d\.]+)\s+(?P<real_unit>\w+)\s+"
    r"(?P<cpu>[\d\.]+)\s+(?P<cpu_unit>\w+)\s+(?P<iterations>\d+)"
)

IMPLEMENTATION_COLORS: Dict[str, str] = {
    'JazzyVector': '#2E86AB',
    'JazzyVectorAggregate': '#1B9AAA',
    'JazzyVectorFifo': '#126872',
    'MapAggregate': '#A23B72',
    'MapFifo': '#6D1A36',
}

BUILD_LINESTYLES: Dict[str, str] = {'native': '-', 'portable': '--'}
BUILD_MARKERS: Dict[str, str] = {'native': 'o', 'portable': 's'}
BUILD_ALPHA: Dict[str, float] = {'native': 0.95, 'portable': 0.75}

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


def detect_hardware_key(script_path: Path) -> Optional[str]:
    """Return the detected hardware key using the helper script."""
    if not script_path.exists():
        return None

    try:
        result = subprocess.run(
            [str(script_path)],
            capture_output=True,
            text=True,
            check=True,
        )
    except (subprocess.CalledProcessError, FileNotFoundError, PermissionError):
        return None

    key = result.stdout.strip()
    return key or None


def parse_benchmark_name(name: str) -> Optional[Dict[str, Any]]:
    """
    Parse a Google Benchmark name into fixture, operation, implementation, and size.

    Supports both fixture-based (Fixture/Operation_Impl/size) and BM_ prefixed names.
    Returns None for aggregate statistics like _BigO or _RMS entries.
    """
    segments = name.split('/')
    if not segments:
        return None

    size_index: Optional[int] = None
    for idx in range(len(segments) - 1, -1, -1):
        try:
            int(segments[idx])
        except ValueError:
            continue
        else:
            size_index = idx
            break

    if size_index is None:
        return None

    try:
        size = int(segments[size_index])
    except ValueError:
        return None

    base_segments = segments[:size_index]
    if not base_segments:
        return None

    first_segment = base_segments[0]

    if first_segment.startswith('BM'):
        if first_segment.endswith('_BigO') or first_segment.endswith('_RMS'):
            return None

        parts = first_segment.split('_')
        if len(parts) < 3:
            return None

        operation_parts = parts[1:-1]
        operation = '_'.join(operation_parts) if operation_parts else parts[1]
        implementation = parts[-1]
        fixture = 'BM'
    else:
        if len(base_segments) < 2:
            return None

        fixture = first_segment
        op_impl = base_segments[1]

        if op_impl.endswith('_BigO') or op_impl.endswith('_RMS'):
            return None

        op_parts = op_impl.split('_')
        if len(op_parts) < 2:
            return None

        operation = op_parts[0]
        implementation = '_'.join(op_parts[1:])

    return {
        'fixture': fixture,
        'operation': operation,
        'implementation': implementation,
        'size': size,
    }


def parse_console_results(path: Path, build_label: str) -> pd.DataFrame:
    """Parse benchmark console output (text) into a DataFrame."""
    records: List[Dict[str, Any]] = []

    if not path.exists():
        raise FileNotFoundError(f"Console result file not found: {path}")

    with path.open() as handle:
        for raw_line in handle:
            line = raw_line.strip()
            match = CONSOLE_LINE_RE.match(line)
            if not match:
                continue

            name = match.group('name')
            parsed = parse_benchmark_name(name)
            if parsed is None:
                continue

            record = {
                **parsed,
                'real_time': float(match.group('real')),
                'real_time_unit': match.group('real_unit'),
                'cpu_time': float(match.group('cpu')),
                'cpu_time_unit': match.group('cpu_unit'),
                'iterations': int(match.group('iterations')),
                'build': build_label,
                'source': 'console',
            }
            records.append(record)

    return pd.DataFrame(records)


def load_best_run_results(
    results_dir: Path,
    native_path: Optional[str] = None,
    portable_path: Optional[str] = None,
    hardware_key: Optional[str] = None,
) -> pd.DataFrame:
    """
    Load stored best run outputs for native and portable builds.

    If explicit paths are not provided, paths are resolved using the hardware key.
    """
    resolved_paths: Dict[str, Path] = {}

    if native_path:
        resolved_paths['native'] = Path(native_path)
    if portable_path:
        resolved_paths['portable'] = Path(portable_path)

    if not resolved_paths:
        # Resolve hardware key automatically if not provided.
        if hardware_key is None:
            hardware_key = detect_hardware_key(Path('./scripts/detect_hardware.sh'))
            if hardware_key is None:
                # Fall back to inspecting the results directory for a single candidate.
                best_files = sorted(
                    f for f in results_dir.glob('*_best.txt') if not f.name.endswith('_portable_best.txt')
                )
                if len(best_files) == 1:
                    hardware_key = best_files[0].name.replace('_best.txt', '')
                else:
                    raise RuntimeError(
                        "Unable to detect hardware key automatically. "
                        "Pass --hardware-key or explicit --native-best/--portable-best paths."
                    )

        resolved_paths['native'] = results_dir / f"{hardware_key}_best.txt"
        resolved_paths['portable'] = results_dir / f"{hardware_key}_portable_best.txt"

    data_frames: List[pd.DataFrame] = []
    for build_label, file_path in resolved_paths.items():
        if not file_path.exists():
            print(f"Warning: Skipping missing {build_label} best results at '{file_path}'.")
            continue

        df = parse_console_results(file_path, build_label)
        if df.empty:
            print(f"Warning: No benchmark data parsed from '{file_path}'.")
            continue

        data_frames.append(df)

    if not data_frames:
        raise RuntimeError("No benchmark data could be loaded from the provided best run files.")

    return pd.concat(data_frames, ignore_index=True)


def prepare_benchmark_dataframe(df: pd.DataFrame) -> pd.DataFrame:
    """Normalize derived columns used by downstream visualizations."""
    if df.empty:
        return df

    df = df.copy()

    if 'build' not in df.columns:
        df['build'] = 'native'

    df['fixture'] = df.get('fixture', pd.Series(dtype=object)).fillna('BM')
    df['real_time_unit'] = df.get('real_time_unit', pd.Series(dtype=object)).fillna('ns')
    df['cpu_time_unit'] = df.get('cpu_time_unit', pd.Series(dtype=object)).fillna('ns')

    def _operation_label(row: pd.Series) -> str:
        fixture = row.get('fixture', 'BM')
        operation = row.get('operation', '')
        if not operation:
            return 'unknown'
        if fixture and fixture != 'BM':
            return f"{fixture}/{operation}"
        return operation

    df['operation_label'] = df.apply(_operation_label, axis=1)
    df['display_implementation'] = df.apply(
        lambda row: f"{row.get('implementation')} ({row.get('build')})", axis=1
    )

    return df


def get_operation_count(operation: str, size: int) -> int:
    """Return the number of logical operations performed for a benchmark run."""
    if size <= 0:
        return 0

    if operation in {'AddOrders', 'UpdateOrders', 'DeleteOrders', 'MixedOps'}:
        return size
    if operation == 'VolumeLookup':
        return size * 2  # N ticks × 2 (bid + ask)
    if operation in {'GetLevelSnapshot', 'GetOrderAtLevel', 'FrontOrderPeek'}:
        return min(size, 20) * 2  # Up to 20 levels × bid/ask

    return size


def parse_benchmark_results(json_path: str, build_label: str = 'native') -> pd.DataFrame:
    """Parse Google Benchmark JSON output into a pandas DataFrame."""
    with open(json_path, 'r') as handle:
        data = json.load(handle)

    benchmarks = data.get('benchmarks', [])

    records: List[Dict[str, Any]] = []
    for bench in benchmarks:
        # Skip aggregate results (mean, median, stddev) - only process iteration results
        if bench.get('run_type') != 'iteration':
            continue

        name = bench.get('name')
        if not name:
            continue

        parsed = parse_benchmark_name(name)
        if parsed is None:
            continue

        record = {
            **parsed,
            'real_time': bench.get('real_time', 0.0),
            'real_time_unit': bench.get('time_unit', 'ns'),
            'cpu_time': bench.get('cpu_time', 0.0),
            'cpu_time_unit': bench.get('time_unit', 'ns'),
            'iterations': bench.get('iterations', 0),
            'build': build_label,
            'source': 'json',
        }
        records.append(record)

    return pd.DataFrame(records)


def create_comparison_plots(df: pd.DataFrame, output_dir: Path):
    """Create comparison plots for all benchmark operations."""
    if df.empty:
        print("No benchmark data available for comparison plots.")
        return

    operations = df['operation_label'].unique()

    # Set style
    plt.style.use('seaborn-v0_8-darkgrid')

    for operation_label in operations:
        op_data = df[df['operation_label'] == operation_label]
        if op_data.empty:
            continue

        fig, (ax_time, ax_ratio) = plt.subplots(1, 2, figsize=(15, 6))
        fig.suptitle(f'{operation_label} Performance Comparison', fontsize=16, fontweight='bold')

        # Plot 1: Time vs Size (log-log scale)
        for (implementation, build_label), impl_data in op_data.groupby(['implementation', 'build']):
            impl_data = impl_data.sort_values('size')
            color = IMPLEMENTATION_COLORS.get(implementation)
            linestyle = BUILD_LINESTYLES.get(build_label, '-')
            marker = BUILD_MARKERS.get(build_label, 'o')
            alpha = BUILD_ALPHA.get(build_label, 0.9)

            ax_time.plot(
                impl_data['size'],
                impl_data['cpu_time'],
                marker=marker,
                linewidth=2,
                markersize=6,
                linestyle=linestyle,
                label=f"{implementation} ({build_label})",
                color=color,
                alpha=alpha,
            )

        ax_time.set_xlabel('Input Size (N)', fontsize=12)
        ax_time.set_ylabel('CPU Time (ns)', fontsize=12)
        ax_time.set_title('Execution Time vs Input Size', fontsize=13)
        ax_time.set_xscale('log', base=2)
        ax_time.set_yscale('log')
        ax_time.legend(fontsize=10)
        ax_time.grid(True, alpha=0.3)

        # Plot 2: Portable vs Native ratio
        ratio_plotted = False
        available_builds = op_data['build'].unique()
        if {'native', 'portable'}.issubset(set(available_builds)):
            for implementation in sorted(op_data['implementation'].unique()):
                native = (
                    op_data[(op_data['implementation'] == implementation) & (op_data['build'] == 'native')]
                    [['size', 'cpu_time']]
                    .rename(columns={'cpu_time': 'cpu_native'})
                )
                portable = (
                    op_data[(op_data['implementation'] == implementation) & (op_data['build'] == 'portable')]
                    [['size', 'cpu_time']]
                    .rename(columns={'cpu_time': 'cpu_portable'})
                )
                merged = pd.merge(native, portable, on='size').sort_values('size')
                if merged.empty:
                    continue

                merged['ratio'] = merged['cpu_portable'] / merged['cpu_native']
                color = IMPLEMENTATION_COLORS.get(implementation)
                ax_ratio.plot(
                    merged['size'],
                    merged['ratio'],
                    marker=BUILD_MARKERS.get('portable', 's'),
                    linewidth=2,
                    markersize=6,
                    linestyle='-',
                    label=f"{implementation} portable/native",
                    color=color,
                )
                ratio_plotted = True

                mean_ratio = merged['ratio'].mean()
                ax_ratio.text(
                    0.05,
                    0.9 - 0.1 * list(sorted(op_data['implementation'].unique())).index(implementation),
                    f"{implementation}: {mean_ratio:.2f}×",
                    transform=ax_ratio.transAxes,
                    fontsize=10,
                    bbox=dict(boxstyle='round', facecolor='white', alpha=0.5),
                )

        if ratio_plotted:
            ax_ratio.axhline(y=1.0, color='red', linestyle='--', linewidth=1, alpha=0.7, label='Parity (1×)')
            ax_ratio.set_xlabel('Input Size (N)', fontsize=12)
            ax_ratio.set_ylabel('Portable / Native CPU time', fontsize=12)
            ax_ratio.set_title('Portable vs Native Ratio', fontsize=13)
            ax_ratio.set_xscale('log', base=2)
            ax_ratio.set_ylim(bottom=0)
            ax_ratio.legend(fontsize=10)
            ax_ratio.grid(True, alpha=0.3)
        else:
            ax_ratio.axis('off')
            ax_ratio.text(
                0.5,
                0.5,
                'Portable/native comparison unavailable',
                transform=ax_ratio.transAxes,
                ha='center',
                va='center',
                fontsize=11,
                bbox=dict(boxstyle='round', facecolor='white', alpha=0.6),
            )

        plt.tight_layout()

        output_path = output_dir / f'{operation_label.replace("/", "_")}_comparison.png'
        plt.savefig(output_path, dpi=150, bbox_inches='tight')
        print(f"Saved plot: {output_path}")
        plt.close()


def create_summary_plot(df: pd.DataFrame, output_dir: Path):
    """Create a summary plot showing all operations."""
    if df.empty:
        print("No benchmark data available for summary plot.")
        return

    operations = sorted(df['operation_label'].unique())
    if not operations:
        print("No operations found for summary plot.")
        return

    n_ops = len(operations)
    n_cols = min(3, n_ops) or 1
    n_rows = math.ceil(n_ops / n_cols)

    fig, axes = plt.subplots(n_rows, n_cols, figsize=(6 * n_cols, 4.5 * n_rows))
    fig.suptitle('Benchmark Performance Summary (native vs portable)', fontsize=18, fontweight='bold')

    if n_ops == 1:
        axes_iter = [axes]
    else:
        axes_iter = axes.flatten()

    legend_handles: Dict[str, Any] = {}

    for idx, operation_label in enumerate(operations):
        if idx >= len(axes_iter):
            break

        ax = axes_iter[idx]
        op_data = df[df['operation_label'] == operation_label]
        if op_data.empty:
            ax.axis('off')
            continue

        for (implementation, build_label), impl_data in op_data.groupby(['implementation', 'build']):
            impl_data = impl_data.sort_values('size')
            color = IMPLEMENTATION_COLORS.get(implementation)
            linestyle = BUILD_LINESTYLES.get(build_label, '-')
            marker = BUILD_MARKERS.get(build_label, 'o')
            alpha = BUILD_ALPHA.get(build_label, 0.9)
            label = f"{implementation} ({build_label})"

            (line,) = ax.plot(
                impl_data['size'],
                impl_data['cpu_time'],
                marker=marker,
                linewidth=2,
                markersize=4,
                linestyle=linestyle,
                color=color,
                alpha=alpha,
            )

            # Keep a single handle per label for the figure legend
            legend_handles.setdefault(label, line)

        ax.set_xlabel('Input Size', fontsize=10)
        ax.set_ylabel('CPU Time (ns)', fontsize=10)
        ax.set_title(operation_label, fontsize=12, fontweight='bold')
        ax.set_xscale('log', base=2)
        ax.set_yscale('log')
        ax.grid(True, alpha=0.3)

    # Hide unused subplots
    for idx in range(len(operations), len(axes_iter)):
        axes_iter[idx].axis('off')

    if legend_handles:
        fig.legend(
            legend_handles.values(),
            legend_handles.keys(),
            loc='upper center',
            ncol=min(len(legend_handles), 4),
            bbox_to_anchor=(0.5, 0.02),
            fontsize=10,
        )

    plt.tight_layout(rect=(0, 0.05, 1, 0.96))

    output_path = output_dir / 'summary_all_operations.png'
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"Saved summary plot: {output_path}")
    plt.close()


def create_bar_chart_comparison(df: pd.DataFrame, output_dir: Path):
    """Create bar charts comparing implementations at specific sizes."""
    if df.empty:
        print("No benchmark data available for bar comparison.")
        return

    max_size = df['size'].max()
    large_size_data = df[df['size'] == max_size].copy()
    if large_size_data.empty:
        print("Skipping bar chart: no data available at the maximum benchmark size.")
        return

    operations = sorted(large_size_data['operation_label'].unique())
    combo_df = large_size_data[['implementation', 'build']].drop_duplicates()
    combos = sorted(combo_df.itertuples(index=False), key=lambda row: (row.implementation, row.build))

    if not combos:
        print("Skipping bar chart: no implementation/build combinations found.")
        return

    x = np.arange(len(operations))
    bar_group_width = 0.8
    width = bar_group_width / len(combos)

    fig_width = max(12, 2.5 * len(operations))
    fig, ax = plt.subplots(figsize=(fig_width, 8))

    for idx, combo in enumerate(combos):
        implementation = combo.implementation
        build_label = combo.build
        subset = large_size_data[
            (large_size_data['implementation'] == implementation) &
            (large_size_data['build'] == build_label)
        ]

        mean_times: List[float] = []
        for op_label in operations:
            row = subset[subset['operation_label'] == op_label]
            if row.empty:
                mean_times.append(np.nan)
                continue

            entry = row.iloc[0]
            op_name = entry['operation']
            size = entry['size']
            operation_count = get_operation_count(op_name, size)
            if operation_count <= 0:
                mean_times.append(np.nan)
            else:
                mean_times.append(entry['cpu_time'] / operation_count)

        offset = (idx - (len(combos) - 1) / 2) * width
        color = IMPLEMENTATION_COLORS.get(implementation)
        alpha = BUILD_ALPHA.get(build_label, 0.85)
        hatch = '//' if build_label == 'portable' else None

        bars = ax.bar(
            x + offset,
            mean_times,
            width,
            label=f"{implementation} ({build_label})",
            color=color,
            alpha=alpha,
            hatch=hatch,
            edgecolor='black',
        )

        for bar, value in zip(bars, mean_times):
            if np.isnan(value) or value <= 0:
                continue
            ax.text(
                bar.get_x() + bar.get_width() / 2.0,
                value,
                f'{value:.1f}',
                ha='center',
                va='bottom',
                fontsize=8,
            )

    ax.set_xlabel('Operation', fontsize=12, fontweight='bold')
    ax.set_ylabel('Mean CPU Time per Operation (ns)', fontsize=12, fontweight='bold')
    ax.set_title(f'Mean Performance per Operation (N={max_size})', fontsize=14, fontweight='bold')
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
        '--use-best',
        action='store_true',
        help='Load stored native and portable best run outputs instead of JSON results'
    )
    parser.add_argument(
        '--hardware-key',
        help='Hardware key used to locate stored best results (default: auto-detect)'
    )
    parser.add_argument(
        '--native-best',
        help='Path to the native best results file (console output)'
    )
    parser.add_argument(
        '--portable-best',
        help='Path to the portable best results file (console output)'
    )
    parser.add_argument(
        '--output-dir',
        default='./benchmark_results',
        help='Directory for output plots (default: ./benchmark_results)'
    )
    parser.add_argument(
        '--best-results-dir',
        default='./benchmark_results',
        help='Directory containing stored best run outputs (default: ./benchmark_results)'
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

    df_raw: pd.DataFrame

    if args.use_best:
        if args.json_input:
            print("Error: --use-best cannot be combined with --json-input.")
            sys.exit(1)

        print("\nLoading stored best run outputs...")
        df_raw = load_best_run_results(
            results_dir=Path(args.best_results_dir),
            native_path=args.native_best,
            portable_path=args.portable_best,
            hardware_key=args.hardware_key,
        )
        source_description = f"best runs ({Path(args.best_results_dir)})"
    else:
        if args.native_best or args.portable_best or args.hardware_key:
            print("Error: --native-best/--portable-best/--hardware-key require --use-best.")
            sys.exit(1)

        json_path = args.json_input or str(output_dir / 'benchmark_results.json')

        # Run benchmarks unless --no-run is specified
        if not args.no_run:
            if not run_benchmarks(args.benchmark_path, json_path):
                sys.exit(1)
        elif not args.json_input:
            print("Error: --no-run requires --json-input")
            sys.exit(1)

        print(f"\nParsing results from: {json_path}")
        df_raw = parse_benchmark_results(json_path)
        source_description = json_path

    df = prepare_benchmark_dataframe(df_raw)

    if df.empty:
        print("Error: No benchmark data found!")
        sys.exit(1)

    operations_list = sorted(df['operation_label'].unique())
    implementations_list = sorted(df['implementation'].unique())
    builds_list = sorted(df['build'].unique())

    print(f"\nLoaded {len(df)} benchmark samples from {source_description}.")
    print(f"Operations: {', '.join(operations_list)}")
    print(f"Implementations: {', '.join(implementations_list)}")
    print(f"Builds: {', '.join(builds_list)}")

    # Create visualizations
    print("\nGenerating plots...")
    create_comparison_plots(df, output_dir)
    create_summary_plot(df, output_dir)
    create_bar_chart_comparison(df, output_dir)

    print(f"\nAll plots saved to: {output_dir}")
    print("Done!")


if __name__ == '__main__':
    main()
