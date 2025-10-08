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

### Visualize Stored Best Runs

If you've captured hardware-specific best runs with `scripts/benchmark_compare.sh`, you can compare the native and portable builds directly:

```bash
python scripts/visualize_benchmarks.py --use-best
```

By default the script auto-detects your hardware key (or falls back to the only `*_best.txt` pair it finds) under `./benchmark_results`. You can override the inputs as needed:

```bash
python scripts/visualize_benchmarks.py \
  --use-best \
  --hardware-key apple_m2 \
  --best-results-dir ./benchmark_results \
  --output-dir ./benchmark_results
```

You can also point to explicit files with `--native-best` and/or `--portable-best` if they live outside the results directory.

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
- **Right plot**: Portable build slowdown vs native (`portable / native` CPU time)

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

--use-best              Load stored native/portable best runs (console output)

--hardware-key KEY      Hardware key used for resolving stored best runs

--native-best FILE      Explicit path to native best results

--portable-best FILE    Explicit path to portable best results

--output-dir DIR        Directory for output plots
                        (default: ./benchmark_results)

--best-results-dir DIR  Directory containing stored best run outputs
                        (default: ./benchmark_results)

--no-run               Skip running benchmarks (requires --json-input)
```

## Understanding the Results

- **Lower is better** for execution time plots
- **Portable ratio** values >1.0 indicate the portable build is slower than native
- Log scales are used to show performance across wide input ranges
- The bar chart shows absolute performance at maximum stress
