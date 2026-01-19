#!/bin/bash
#
# Tag the current baseline in git
#
# Reads the tag from baseline.json and creates an annotated git tag.
#
# Usage:
#   ./scripts/tag-baseline.sh          # Create tag locally
#   ./scripts/tag-baseline.sh --push   # Create and push tag to remote
#
# Environment variables:
#   BASELINE_PATH: Path to baseline.json (default: metrics/baseline.json)
#

# Exit on error
set -e

# Get the directory where the script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
PROJECT_ROOT="$( dirname "$SCRIPT_DIR" )"

# Configuration
BASELINE_PATH="${BASELINE_PATH:-$PROJECT_ROOT/metrics/baseline.json}"
PUSH_TAG=false

# Parse arguments
for arg in "$@"; do
    case $arg in
        --push)
            PUSH_TAG=true
            shift
            ;;
        --help|-h)
            echo "Usage: $0 [--push]"
            echo ""
            echo "Create an annotated git tag from baseline.json"
            echo ""
            echo "Options:"
            echo "  --push    Push the tag to remote after creating"
            echo "  --help    Show this help message"
            echo ""
            echo "Environment variables:"
            echo "  BASELINE_PATH    Path to baseline.json (default: metrics/baseline.json)"
            exit 0
            ;;
        *)
            echo "Unknown option: $arg"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Check if baseline.json exists
if [ ! -f "$BASELINE_PATH" ]; then
    echo "Error: Baseline file not found: $BASELINE_PATH"
    exit 1
fi

# Check for jq
if ! command -v jq &> /dev/null; then
    echo "Error: jq is required but not installed."
    echo "Install with: brew install jq (macOS) or apt-get install jq (Linux)"
    exit 1
fi

# Extract tag from baseline.json
TAG=$(jq -r '.tag // empty' "$BASELINE_PATH")

if [ -z "$TAG" ] || [ "$TAG" = "null" ]; then
    echo "Error: No tag found in baseline.json"
    echo "Run 'node scripts/update-baseline.js' first to generate a tag"
    exit 1
fi

# Function to increment patch version
increment_patch_version() {
    local version=$1
    # Extract version parts (assuming format: baseline-vMAJOR.MINOR.PATCH)
    if [[ $version =~ baseline-v([0-9]+)\.([0-9]+)\.([0-9]+) ]]; then
        local major="${BASH_REMATCH[1]}"
        local minor="${BASH_REMATCH[2]}"
        local patch="${BASH_REMATCH[3]}"
        patch=$((patch + 1))
        echo "baseline-v${major}.${minor}.${patch}"
    else
        echo "$version"
    fi
}

# Fetch remote tags to check for conflicts
echo "Fetching remote tags..."
git fetch --tags origin 2>/dev/null || true

# Check if tag already exists (local or remote)
BASELINE_COMMIT=$(jq -r '.commit // empty' "$BASELINE_PATH")
ORIGINAL_TAG="$TAG"

# Function to check if tag exists locally or remotely
tag_exists() {
    local tag=$1
    # Check local
    if git rev-parse "$tag" >/dev/null 2>&1; then
        return 0
    fi
    # Check remote
    if git ls-remote --tags origin | grep -q "refs/tags/$tag$"; then
        return 0
    fi
    return 1
}

while tag_exists "$TAG"; do
    # Check if local tag exists and points to the same commit
    if git rev-parse "$TAG" >/dev/null 2>&1; then
        TAG_COMMIT=$(git rev-parse "$TAG^{}")

        if [ "$TAG_COMMIT" = "$BASELINE_COMMIT" ]; then
            echo "Tag '$TAG' already exists locally and points to the correct commit"
            if [ "$PUSH_TAG" = true ]; then
                echo "Pushing existing tag..."
                git push origin "$TAG" 2>&1 || echo "Tag already exists on remote"
            fi
            exit 0
        else
            echo "Tag '$TAG' already exists locally (points to ${TAG_COMMIT:0:8})"
        fi
    else
        echo "Tag '$TAG' already exists on remote"
    fi

    # Auto-increment
    NEW_TAG=$(increment_patch_version "$TAG")
    if [ "$NEW_TAG" = "$TAG" ]; then
        echo "Error: Could not auto-increment version"
        exit 1
    fi
    echo "Auto-incrementing to: $NEW_TAG"
    TAG="$NEW_TAG"
done

if [ "$TAG" != "$ORIGINAL_TAG" ]; then
    echo "Using incremented tag: $TAG (was $ORIGINAL_TAG)"
fi

# Extract metadata for tag message
COMMIT=$(jq -r '.commit // "unknown"' "$BASELINE_PATH")
TIMESTAMP=$(jq -r '.generated_at // "unknown"' "$BASELINE_PATH")
OVERALL_SCORE=$(jq -r '.metrics.overallAlignment // 0' "$BASELINE_PATH")
CONSECUTIVE=$(jq -r '.consecutive_regressions // 0' "$BASELINE_PATH")

# Format overall score as percentage
SCORE_PCT=$(echo "$OVERALL_SCORE * 100" | bc -l 2>/dev/null | xargs printf "%.1f" 2>/dev/null || echo "?")

# Build tag message
TAG_MESSAGE="Baseline $TAG

Overall Alignment: ${SCORE_PCT}%
Timestamp: $TIMESTAMP
Commit: $COMMIT
Consecutive Regressions: $CONSECUTIVE"

echo "Creating tag: $TAG"
echo "---"
echo "$TAG_MESSAGE"
echo "---"

# Create annotated tag
git tag -a "$TAG" -m "$TAG_MESSAGE"

echo ""
echo "Tag '$TAG' created successfully"

# Push if requested
if [ "$PUSH_TAG" = true ]; then
    echo "Pushing tag to remote..."
    git push origin "$TAG"
    echo "Tag pushed to origin"
fi

echo ""
echo "Done!"
