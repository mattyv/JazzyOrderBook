#!/bin/bash

# Script to install git hooks

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
HOOKS_DIR="${REPO_ROOT}/.git/hooks"

echo "Installing git hooks..."

# Install pre-commit hook
if [ -f "${SCRIPT_DIR}/pre-commit" ]; then
    cp "${SCRIPT_DIR}/pre-commit" "${HOOKS_DIR}/pre-commit"
    chmod +x "${HOOKS_DIR}/pre-commit"
    echo "✓ Pre-commit hook installed"
else
    echo "✗ Pre-commit hook not found"
fi

echo "Git hooks installation complete!"