#!/bin/bash
# Simple hardware detection

set -e

# Get hardware key
get_hardware_key() {
    if command -v lscpu >/dev/null 2>&1; then
        # Linux with lscpu
        CPU=$(lscpu | grep "Model name" | cut -d: -f2 | xargs | tr ' ' '_' | tr '[:upper:]' '[:lower:]')
        echo "$CPU"
    elif [[ "$(uname)" == "Darwin" ]]; then
        # macOS
        CPU=$(sysctl -n machdep.cpu.brand_string 2>/dev/null | tr ' ' '_' | tr '[:upper:]' '[:lower:]')
        echo "$CPU"
    else
        # Fallback
        echo "unknown_cpu"
    fi
}

get_hardware_key