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

    # Pick a sensible width for side-by-side diff to reduce truncation without
    # exploding the spacing when the terminal is narrow.
    if command -v tput >/dev/null 2>&1; then
        TERM_COLS=$(tput cols)
    else
        TERM_COLS=${COLUMNS:-0}
    fi

    MAX_LINE_BEST=$(wc -L < "$BEST_FILE")
    MAX_LINE_NEW=$(wc -L < "$NEW_FILE")

    # Leave breathing room beyond raw lengths to account for diff separators
    # and spacing that wc -L does not capture.
    PADDING=32
    REQUIRED_WIDTH=$((MAX_LINE_BEST + MAX_LINE_NEW + PADDING))

    # Clamp width to a reasonable range before considering terminal width.
    MIN_WIDTH=120
    MAX_WIDTH=600

    DIFF_WIDTH=$REQUIRED_WIDTH

    if [[ "$DIFF_WIDTH" -lt "$MIN_WIDTH" ]]; then
        DIFF_WIDTH=$MIN_WIDTH
    elif [[ "$DIFF_WIDTH" -gt "$MAX_WIDTH" ]]; then
        DIFF_WIDTH=$MAX_WIDTH
    fi

    WIDTH_NOTE=""
    if [[ -n "$TERM_COLS" && "$TERM_COLS" -gt 0 && "$TERM_COLS" -lt "$DIFF_WIDTH" ]]; then
        DIFF_WIDTH=$TERM_COLS
        WIDTH_NOTE="(diff width limited to terminal width; consider widening the window for full view)"
    fi

    set +e
    if command -v expand >/dev/null 2>&1; then
        diff -y --width="$DIFF_WIDTH" "$BEST_FILE" "$NEW_FILE" | expand -t 4
        DIFF_EXIT=${PIPESTATUS[0]}
    else
        diff -y --width="$DIFF_WIDTH" "$BEST_FILE" "$NEW_FILE"
        DIFF_EXIT=$?
    fi
    set -e

    if [[ "$DIFF_EXIT" -gt 1 ]]; then
        exit "$DIFF_EXIT"
    fi

    if [[ -n "$WIDTH_NOTE" ]]; then
        echo ""
        echo "$WIDTH_NOTE"
    fi
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
