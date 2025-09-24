#!/bin/bash
# Simple benchmark comparison using diff

set -e

# Get hardware key and setup paths
HARDWARE_KEY=$(./scripts/detect_hardware.sh)
RESULTS_DIR="benchmark_results"
BEST_FILE="$RESULTS_DIR/${HARDWARE_KEY}_best.txt"
NEW_FILE="$RESULTS_DIR/${HARDWARE_KEY}_new.txt"

mkdir -p "$RESULTS_DIR"

# Run new benchmark
echo "Running benchmark for: $HARDWARE_KEY"
./build/benchmarks/benchmarks > "$NEW_FILE"

echo ""
echo "=== COMPARISON ==="

if [[ -f "$BEST_FILE" ]]; then
    echo "Comparing against existing best:"
    echo "BEST (left) vs NEW (right):"
    diff -y "$BEST_FILE" "$NEW_FILE" || true
    echo ""
    echo "Which results do you want to keep?"
    echo "  b) Keep BEST (existing)"
    echo "  n) Keep NEW (just ran)"
    echo "  q) Quit without saving"
    read -r choice

    case "$choice" in
        [Nn]*)
            mv "$NEW_FILE" "$BEST_FILE"
            echo "New results saved as best"
            ;;
        [Qq]*)
            rm "$NEW_FILE"
            echo "Exiting without saving"
            ;;
        *)
            rm "$NEW_FILE"
            echo "Keeping existing best"
            ;;
    esac
else
    echo "No existing best found - saving as best"
    mv "$NEW_FILE" "$BEST_FILE"
    echo "Results saved as best"
fi