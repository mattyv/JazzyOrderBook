#!/bin/bash
# Simple benchmark comparison using diff

set -e

usage() {
    cat <<'EOF'
Usage: ./scripts/benchmark_compare.sh [OPTIONS]

Run the benchmark suite (native by default) and compare the results against
the best stored run for this hardware. Results are stored in benchmark_results/
using your hardware key.

Options:
  -p, --portable        Run the portable benchmark binary and compare it against
                        the saved portable best results.
  -c, --compare-best    Show the saved native and portable best results side by side.
                        Cannot be combined with --portable.
  -h, --help            Print this help message and exit.

Run without options to execute the native benchmark and optionally promote the
new run to the best results for your hardware.
EOF
}

side_by_side_diff() {
    local left_file="$1"
    local right_file="$2"
    local left_label="$3"
    local right_label="$4"

    echo "$left_label (left) vs $right_label (right):"

    local term_cols
    if command -v tput >/dev/null 2>&1; then
        term_cols=$(tput cols)
    else
        term_cols=${COLUMNS:-0}
    fi

    local max_line_left max_line_right padding required_width min_width max_width diff_width width_note diff_exit
    max_line_left=$(wc -L < "$left_file")
    max_line_right=$(wc -L < "$right_file")

    padding=32
    required_width=$((max_line_left + max_line_right + padding))

    min_width=120
    max_width=600

    diff_width=$required_width

    if [[ "$diff_width" -lt "$min_width" ]]; then
        diff_width=$min_width
    elif [[ "$diff_width" -gt "$max_width" ]]; then
        diff_width=$max_width
    fi

    width_note=""
    if [[ -n "$term_cols" && "$term_cols" -gt 0 && "$term_cols" -lt "$diff_width" ]]; then
        diff_width=$term_cols
        width_note="(diff width limited to terminal width; consider widening the window for full view)"
    fi

    set +e
    if command -v expand >/dev/null 2>&1; then
        diff -y --width="$diff_width" "$left_file" "$right_file" | expand -t 4
        diff_exit=${PIPESTATUS[0]}
    else
        diff -y --width="$diff_width" "$left_file" "$right_file"
        diff_exit=$?
    fi
    set -e

    if [[ "$diff_exit" -gt 1 ]]; then
        exit "$diff_exit"
    fi

    if [[ -n "$width_note" ]]; then
        echo ""
        echo "$width_note"
    fi
}

MODE_LABEL="native"
BENCH_BINARY="./build/benchmarks/benchmarks"
RESULT_SUFFIX=""
COMPARE_BEST=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--portable)
            MODE_LABEL="portable"
            BENCH_BINARY="./build/benchmarks/benchmarks_portable"
            RESULT_SUFFIX="_portable"
            shift
            ;;
        -c|--compare-best)
            COMPARE_BEST=true
            shift
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo ""
            usage
            exit 1
            ;;
    esac
done

if [[ "$COMPARE_BEST" == "true" && -n "$RESULT_SUFFIX" ]]; then
    echo "Error: --compare-best cannot be combined with --portable."
    exit 1
fi

if [[ "$COMPARE_BEST" != "true" && ! -x "$BENCH_BINARY" ]]; then
    echo "Error: benchmark binary '$BENCH_BINARY' not found or not executable."
    echo "Make sure the project (including the portable targets) has been built."
    exit 1
fi

# Get hardware key and setup paths
HARDWARE_KEY=$(./scripts/detect_hardware.sh)
RESULTS_DIR="benchmark_results"
BEST_FILE="$RESULTS_DIR/${HARDWARE_KEY}${RESULT_SUFFIX}_best.txt"
NEW_FILE="$RESULTS_DIR/${HARDWARE_KEY}${RESULT_SUFFIX}_new.txt"

mkdir -p "$RESULTS_DIR"

if [[ "$COMPARE_BEST" == "true" ]]; then
    NATIVE_BEST_FILE="$RESULTS_DIR/${HARDWARE_KEY}_best.txt"
    PORTABLE_BEST_FILE="$RESULTS_DIR/${HARDWARE_KEY}_portable_best.txt"

    if [[ ! -f "$NATIVE_BEST_FILE" ]]; then
        echo "No native best results found at '$NATIVE_BEST_FILE'. Run the native benchmark first."
        exit 1
    fi

    if [[ ! -f "$PORTABLE_BEST_FILE" ]]; then
        echo "No portable best results found at '$PORTABLE_BEST_FILE'. Run the portable benchmark first."
        exit 1
    fi

    echo "Comparing stored best results for: $HARDWARE_KEY"
    side_by_side_diff "$NATIVE_BEST_FILE" "$PORTABLE_BEST_FILE" "BEST (native)" "BEST (portable)"
    echo ""
    exit 0
fi

# Run new benchmark
echo "Running $MODE_LABEL benchmark for: $HARDWARE_KEY"
"$BENCH_BINARY" > "$NEW_FILE"

echo ""
echo "=== COMPARISON ==="

if [[ -f "$BEST_FILE" ]]; then
    echo "Comparing against existing $MODE_LABEL best:"
    side_by_side_diff "$BEST_FILE" "$NEW_FILE" "BEST ($MODE_LABEL)" "NEW ($MODE_LABEL)"
    echo ""
    echo "Which $MODE_LABEL results do you want to keep?"
    echo "  b) Keep BEST (existing)"
    echo "  n) Keep NEW (just ran)"
    echo "  q) Quit without saving"
    read -r choice

    case "$choice" in
        [Nn]*)
            mv "$NEW_FILE" "$BEST_FILE"
            echo "New $MODE_LABEL results saved as best"
            ;;
        [Qq]*)
            rm "$NEW_FILE"
            echo "Exiting without saving $MODE_LABEL results"
            ;;
        *)
            rm "$NEW_FILE"
            echo "Keeping existing $MODE_LABEL best"
            ;;
    esac
else
    echo "No existing $MODE_LABEL best found - saving as best"
    mv "$NEW_FILE" "$BEST_FILE"
    echo "$MODE_LABEL results saved as best"
fi
