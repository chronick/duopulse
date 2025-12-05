#!/bin/bash

# Exit on error
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( dirname "$SCRIPT_DIR" )"
GENERATOR_DIR="$SCRIPT_DIR/manual_generator"

# Check for uv
if [ -d "$HOME/.local/bin" ]; then
    export PATH="$HOME/.local/bin:$PATH"
fi

if ! command -v uv &> /dev/null; then
    echo "Error: uv is not installed. Please install it first."
    echo "Visit https://github.com/astral-sh/uv for installation instructions."
    exit 1
fi

# MacOS specific setup for WeasyPrint/Cairo/Pango
if [[ "$OSTYPE" == "darwin"* ]]; then
    # Add Homebrew lib path to DYLD_FALLBACK_LIBRARY_PATH if it exists
    if [ -d "/opt/homebrew/lib" ]; then
        export DYLD_FALLBACK_LIBRARY_PATH="/opt/homebrew/lib:$DYLD_FALLBACK_LIBRARY_PATH"
    elif [ -d "/usr/local/lib" ]; then
        export DYLD_FALLBACK_LIBRARY_PATH="/usr/local/lib:$DYLD_FALLBACK_LIBRARY_PATH"
    fi
fi

echo "Generating manual..."
cd "$GENERATOR_DIR"
uv run generate.py

echo "Done! Manuals generated in $GENERATOR_DIR/output/"

# Optional post-generation hook
POST_HOOK="$GENERATOR_DIR/post_generate.sh"
if [ -f "$POST_HOOK" ]; then
    echo "Running post-generation hook..."
    GENERATED_FILE="$GENERATOR_DIR/output/manual.pdf"
    if [ -f "$GENERATED_FILE" ]; then
        "$POST_HOOK" "$GENERATED_FILE"
    else
        echo "Warning: Generated file not found: $GENERATED_FILE"
    fi
else
    echo "Tip: You can add a post-generation hook at $POST_HOOK to automatically process the generated manual."
fi

