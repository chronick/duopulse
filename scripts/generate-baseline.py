#!/usr/bin/env python3
"""
Generate baseline metrics JSON from evals output.

This script wraps the evals expressiveness.json output with metadata
(commit hash, timestamp, version) for baseline tracking.

Usage:
    python scripts/generate-baseline.py [evals_path] [output_path]

If no arguments provided:
    evals_path defaults to tools/evals/public/expressiveness.json
    output_path defaults to metrics/baseline.json
"""

import json
import sys
import os
import subprocess
from datetime import datetime

def get_git_commit():
    """Get current git commit hash."""
    try:
        result = subprocess.run(
            ['git', 'rev-parse', 'HEAD'],
            capture_output=True, text=True, check=True
        )
        return result.stdout.strip()
    except subprocess.CalledProcessError:
        return "unknown"

def generate_baseline(evals_path: str, output_path: str) -> None:
    """Generate baseline JSON from evals output."""

    if not os.path.exists(evals_path):
        print(f"Error: Evals file not found: {evals_path}")
        print("Run 'make evals' first to generate metrics.")
        sys.exit(1)

    with open(evals_path, 'r') as f:
        evals_data = json.load(f)

    baseline = {
        "version": "1.0.0",
        "generated_at": datetime.utcnow().strftime('%Y-%m-%dT%H:%M:%SZ'),
        "commit": get_git_commit(),
        "metrics": evals_data
    }

    # Ensure output directory exists
    os.makedirs(os.path.dirname(output_path), exist_ok=True)

    with open(output_path, 'w') as f:
        json.dump(baseline, f, indent=2)

    print(f"Baseline saved to {output_path}")
    print(f"  Commit: {baseline['commit'][:8]}...")
    print(f"  Generated: {baseline['generated_at']}")

def main():
    # Default paths
    evals_path = 'tools/evals/public/data/expressiveness.json'
    output_path = 'metrics/baseline.json'

    # Override from command line
    if len(sys.argv) > 1:
        evals_path = sys.argv[1]
    if len(sys.argv) > 2:
        output_path = sys.argv[2]

    generate_baseline(evals_path, output_path)

if __name__ == '__main__':
    main()
