# Benchmark Visualization

Python script to visualize JazzyOrderBook benchmark results and compare performance against MapOrderBook.

## Installation

### Option 1: Using Virtual Environment (Recommended)

```bash
# Create virtual environment
python3 -m venv scripts/venv

# Activate it
source scripts/venv/bin/activate  # On macOS/Linux
# or
scripts\venv\Scripts\activate     # On Windows

# Install dependencies
pip install -r scripts/requirements.txt
```

### Option 2: Global Install

```bash
pip install -r scripts/requirements.txt
```

Or install packages individually:

```bash
pip install matplotlib pandas numpy
```

## Usage

### Basic Usage

Run benchmarks and generate plots:

```bash
python scripts/visualize_benchmarks.py
```

This will:
1. Run the benchmark executable from `./build/benchmarks/benchmarks`
2. Save results to `./benchmark_results/benchmark_results.json`
3. Generate visualization plots in `./benchmark_results/`

### Custom Benchmark Path

If your benchmark executable is in a different location:

```bash
python scripts/visualize_benchmarks.py --benchmark-path ./path/to/benchmarks
```

### Use Existing JSON Results

To visualize previously saved benchmark results without re-running:

```bash
python scripts/visualize_benchmarks.py --json-input results.json --no-run
```

### Custom Output Directory

Specify where to save the plots:

```bash
python scripts/visualize_benchmarks.py --output-dir ./my_results
```

## Generated Plots

The script generates the following visualizations:

### Individual Operation Comparisons

For each benchmark operation (e.g., `AddOrders`, `UpdateOrders`, etc.):
- **Left plot**: Execution time vs input size (log-log scale)
- **Right plot**: Performance speedup ratio between implementations

Files: `<operation>_comparison.png`

### Summary Plot

A grid showing all operations in a single view for quick comparison.

File: `summary_all_operations.png`

### Bar Chart

Direct comparison of both implementations at the largest input size.

File: `bar_comparison.png`

## Example

```bash
# Build the project
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Setup virtual environment
python3 -m venv scripts/venv
source scripts/venv/bin/activate

# Install dependencies
pip install -r scripts/requirements.txt

# Run visualization
python scripts/visualize_benchmarks.py

# View results
open benchmark_results/summary_all_operations.png
```

## Options

```
--benchmark-path PATH   Path to benchmark executable
                        (default: ./build/benchmarks/benchmarks)

--json-input FILE       Use existing JSON file instead of running benchmarks

--output-dir DIR        Directory for output plots
                        (default: ./benchmark_results)

--no-run               Skip running benchmarks (requires --json-input)
```

## Understanding the Results

- **Lower is better** for execution time plots
- **Higher is better** for speedup ratios (>1.0 means JazzyOrderBook is faster)
- Log scales are used to show performance across wide input ranges
- The bar chart shows absolute performance at maximum stress
