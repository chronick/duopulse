#!/bin/bash
#
# ensemble-search.sh
#
# Orchestrates ensemble search for optimal weight configuration.
# Generates candidates, evaluates in parallel, runs tournament selection,
# and iterates until convergence.
#
# Usage:
#   ./ensemble-search.sh "improve syncopation"
#   ./ensemble-search.sh --goal "improve syncopation" --candidates 4 --rounds 3
#   ./ensemble-search.sh --dry-run
#
# Workflow:
#   1. Read baseline from metrics/baseline.json
#   2. Parse goal to identify target metric
#   3. For each round (max 5):
#      a. Generate N candidates
#      b. Evaluate each (make evals-generate, evals-evaluate)
#      c. Tournament select top K
#      d. Check convergence
#      e. If converged, break
#      f. Else, crossover winners for next round
#   4. Output final winner
#   5. Log search history to docs/design/iterations/ensemble-{timestamp}.md

set -e

# Project paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
METRICS_DIR="$PROJECT_ROOT/metrics"
CONFIG_DIR="$PROJECT_ROOT/config/weights"
ITERATIONS_DIR="$PROJECT_ROOT/docs/design/iterations"
TEMP_DIR="/tmp/ensemble-search-$$"

# Default configuration
CANDIDATES_PER_ROUND=4
MAX_ROUNDS=5
TOP_K=2
CONVERGENCE_THRESHOLD=0.01
CONVERGENCE_WINDOW=3
DRY_RUN=false
VERBOSE=false
GOAL=""
TARGET_METRIC=""

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            --goal|-g)
                GOAL="$2"
                shift 2
                ;;
            --candidates|-n)
                CANDIDATES_PER_ROUND="$2"
                shift 2
                ;;
            --rounds|-r)
                MAX_ROUNDS="$2"
                shift 2
                ;;
            --top-k|-k)
                TOP_K="$2"
                shift 2
                ;;
            --dry-run)
                DRY_RUN=true
                shift
                ;;
            --verbose|-v)
                VERBOSE=true
                shift
                ;;
            --help|-h)
                show_help
                exit 0
                ;;
            *)
                # Positional argument = goal
                if [[ -z "$GOAL" ]]; then
                    GOAL="$1"
                fi
                shift
                ;;
        esac
    done
}

show_help() {
    cat << EOF
Usage: ensemble-search.sh [options] [goal]

Orchestrates ensemble search for optimal weight configuration.

Options:
  --goal, -g <text>      Improvement goal (e.g., "improve syncopation")
  --candidates, -n <N>   Candidates per round (default: 4)
  --rounds, -r <N>       Maximum rounds (default: 5)
  --top-k, -k <N>        Winners per round (default: 2)
  --dry-run              Show what would be done without executing
  --verbose, -v          Verbose output
  --help, -h             Show this help

Examples:
  ./ensemble-search.sh "improve syncopation"
  ./ensemble-search.sh --goal "balance density" --candidates 6 --rounds 3
  ./ensemble-search.sh --dry-run "improve regularity"

Target Metrics:
  syncopation, density, velocityRange, voiceSeparation, regularity
EOF
}

# Parse goal to extract target metric
parse_goal() {
    local goal="$1"
    local goal_lower=$(echo "$goal" | tr '[:upper:]' '[:lower:]')

    # Extract metric from goal text
    if [[ "$goal_lower" == *"syncopation"* ]]; then
        TARGET_METRIC="syncopation"
    elif [[ "$goal_lower" == *"density"* ]]; then
        TARGET_METRIC="density"
    elif [[ "$goal_lower" == *"velocity"* ]] || [[ "$goal_lower" == *"velocityrange"* ]]; then
        TARGET_METRIC="velocityRange"
    elif [[ "$goal_lower" == *"separation"* ]] || [[ "$goal_lower" == *"voiceseparation"* ]]; then
        TARGET_METRIC="voiceSeparation"
    elif [[ "$goal_lower" == *"regularity"* ]]; then
        TARGET_METRIC="regularity"
    elif [[ "$goal_lower" == *"composite"* ]]; then
        TARGET_METRIC="composite"
    else
        log_error "Could not parse target metric from goal: $goal"
        log_info "Use one of: syncopation, density, velocityRange, voiceSeparation, regularity"
        exit 1
    fi

    log_info "Target metric: $TARGET_METRIC"
}

# Setup temp directory
setup_temp() {
    mkdir -p "$TEMP_DIR/candidates"
    mkdir -p "$TEMP_DIR/results"
    mkdir -p "$TEMP_DIR/rounds"
}

# Cleanup temp directory
cleanup_temp() {
    if [[ -d "$TEMP_DIR" ]]; then
        rm -rf "$TEMP_DIR"
    fi
}

trap cleanup_temp EXIT

# Generate candidate weight configurations
generate_candidates() {
    local round=$1
    local strategy=$2
    local parent_files="$3"

    log_info "Generating $CANDIDATES_PER_ROUND candidates (strategy: $strategy)..."

    local args="--count $CANDIDATES_PER_ROUND --strategy $strategy"

    if [[ "$strategy" == "gradient" ]]; then
        args="$args --target $TARGET_METRIC"
    elif [[ "$strategy" == "crossover" ]] && [[ -n "$parent_files" ]]; then
        args="$args --parents $parent_files"
    fi

    if $DRY_RUN; then
        log_info "[DRY RUN] Would run: node $SCRIPT_DIR/generate-candidates.js $args"
        # Generate dummy candidates for dry run
        echo '[{"id":"A","weights":{}},{"id":"B","weights":{}},{"id":"C","weights":{}},{"id":"D","weights":{}}]'
        return
    fi

    node "$SCRIPT_DIR/generate-candidates.js" $args
}

# Write candidate config to file
write_candidate_config() {
    local candidate_json="$1"
    local candidate_id="$2"
    local output_file="$TEMP_DIR/candidates/candidate-${candidate_id}.json"

    echo "$candidate_json" | jq -r ".config" > "$output_file"
    echo "$output_file"
}

# Evaluate a single candidate
evaluate_candidate() {
    local candidate_id="$1"
    local config_file="$2"
    local result_file="$TEMP_DIR/results/result-${candidate_id}.json"

    if $DRY_RUN; then
        log_info "[DRY RUN] Would evaluate candidate $candidate_id"
        # Create dummy result
        echo "{\"id\":\"$candidate_id\",\"metrics\":{\"syncopation\":0.5,\"density\":0.5}}" > "$result_file"
        return
    fi

    log_info "Evaluating candidate $candidate_id..."

    # Generate weights header from candidate config
    python3 "$PROJECT_ROOT/scripts/generate-weights-header.py" \
        "$config_file" \
        "$PROJECT_ROOT/inc/generated/weights_config.h"

    # Build pattern_viz with new weights
    make -C "$PROJECT_ROOT" pattern-viz > /dev/null 2>&1

    # Generate patterns and evaluate
    cd "$PROJECT_ROOT/tools/evals"
    PATTERN_VIZ="$PROJECT_ROOT/build/pattern_viz" node generate-patterns.js > /dev/null 2>&1
    node evaluate-expressiveness.js > /dev/null 2>&1
    cd "$PROJECT_ROOT"

    # Extract metrics from expressiveness.json
    local metrics=$(cat "$PROJECT_ROOT/tools/evals/public/data/expressiveness.json")

    # Build result object
    jq -n \
        --arg id "$candidate_id" \
        --argjson metrics "$metrics" \
        '{id: $id, metrics: $metrics}' > "$result_file"
}

# Run parallel evaluation of all candidates
evaluate_candidates_parallel() {
    local candidates_json="$1"
    local num_candidates=$(echo "$candidates_json" | jq -r 'length')

    log_info "Evaluating $num_candidates candidates in parallel..."

    # Extract each candidate and evaluate
    local pids=()

    for i in $(seq 0 $((num_candidates - 1))); do
        local candidate=$(echo "$candidates_json" | jq -r ".[$i]")
        local candidate_id=$(echo "$candidate" | jq -r '.id')

        # Write config file
        echo "$candidate" | jq -r '.config' > "$TEMP_DIR/candidates/candidate-${candidate_id}.json"

        if $DRY_RUN; then
            evaluate_candidate "$candidate_id" "$TEMP_DIR/candidates/candidate-${candidate_id}.json" &
        else
            # Run evaluation in background
            (evaluate_candidate "$candidate_id" "$TEMP_DIR/candidates/candidate-${candidate_id}.json") &
        fi
        pids+=($!)
    done

    # Wait for all evaluations to complete
    for pid in "${pids[@]}"; do
        wait "$pid"
    done

    log_success "All candidates evaluated"
}

# Collect results into single JSON array
collect_results() {
    local results="[]"

    for result_file in "$TEMP_DIR/results"/*.json; do
        if [[ -f "$result_file" ]]; then
            local result=$(cat "$result_file")
            results=$(echo "$results" | jq --argjson r "$result" '. + [$r]')
        fi
    done

    echo "$results"
}

# Run tournament selection
run_tournament() {
    local results_file="$1"

    log_info "Running tournament selection (target: $TARGET_METRIC, top-k: $TOP_K)..."

    local args="--results $results_file --target $TARGET_METRIC --top-k $TOP_K"

    if $VERBOSE; then
        args="$args --verbose"
    fi

    if $DRY_RUN; then
        log_info "[DRY RUN] Would run: node $SCRIPT_DIR/tournament-select.js $args"
        echo '{"winners":["A","B"],"rankings":[{"id":"A","rank":1},{"id":"B","rank":2}]}'
        return
    fi

    node "$SCRIPT_DIR/tournament-select.js" $args
}

# Check convergence
check_convergence() {
    local history_file="$1"
    local current_best="$2"

    if [[ ! -f "$history_file" ]]; then
        echo "$current_best" > "$history_file"
        echo "false"
        return
    fi

    local history=$(cat "$history_file")
    local num_entries=$(echo "$history" | wc -l)

    # Add current best to history
    echo "$current_best" >> "$history_file"

    if [[ $num_entries -lt $CONVERGENCE_WINDOW ]]; then
        echo "false"
        return
    fi

    # Check if improvement is below threshold for last N rounds
    local recent=$(tail -n "$CONVERGENCE_WINDOW" "$history_file")
    local max_val=$(echo "$recent" | sort -rn | head -1)
    local min_val=$(echo "$recent" | sort -n | head -1)
    local range=$(echo "$max_val - $min_val" | bc -l)

    if (( $(echo "$range < $CONVERGENCE_THRESHOLD" | bc -l) )); then
        echo "true"
    else
        echo "false"
    fi
}

# Generate iteration log
generate_log() {
    local timestamp="$1"
    local winners="$2"
    local round_history="$3"
    local final_metrics="$4"

    local log_file="$ITERATIONS_DIR/ensemble-${timestamp}.md"

    cat > "$log_file" << EOF
---
iteration_id: ensemble-${timestamp}
goal: "$GOAL"
target_metric: $TARGET_METRIC
status: complete
started_at: ${timestamp}T00:00:00Z
completed_at: $(date -u +"%Y-%m-%dT%H:%M:%SZ")
---

# Ensemble Search: $GOAL

## Configuration

- Candidates per round: $CANDIDATES_PER_ROUND
- Max rounds: $MAX_ROUNDS
- Top K selection: $TOP_K
- Convergence threshold: $CONVERGENCE_THRESHOLD
- Convergence window: $CONVERGENCE_WINDOW

## Target Metric

**$TARGET_METRIC**

## Round History

$round_history

## Final Winners

$winners

## Final Metrics

\`\`\`json
$final_metrics
\`\`\`

---

*Generated by ensemble-search.sh on $(date)*
EOF

    log_success "Log written to: $log_file"
}

# Main search loop
main() {
    parse_args "$@"

    if [[ -z "$GOAL" ]]; then
        log_error "Goal is required"
        show_help
        exit 1
    fi

    parse_goal "$GOAL"
    setup_temp

    local timestamp=$(date +"%Y-%m-%d-%H%M%S")
    local round_history=""
    local history_file="$TEMP_DIR/convergence_history.txt"
    local current_winners=""
    local best_improvement=0

    log_info "Starting ensemble search"
    log_info "Goal: $GOAL"
    log_info "Max rounds: $MAX_ROUNDS"
    log_info "Candidates per round: $CANDIDATES_PER_ROUND"
    echo ""

    for round in $(seq 1 $MAX_ROUNDS); do
        log_info "========== Round $round/$MAX_ROUNDS =========="

        # Clear results from previous round
        rm -f "$TEMP_DIR/results"/*.json

        # Determine strategy for this round
        local strategy="random"
        local parents=""

        if [[ $round -eq 1 ]]; then
            # First round: mix of random and gradient
            strategy="gradient"
        elif [[ -n "$current_winners" ]]; then
            # Subsequent rounds: crossover winners + mutations
            if [[ $(echo "$current_winners" | jq -r 'length') -ge 2 ]]; then
                strategy="crossover"
                # Get config files for top 2 winners
                local w1=$(echo "$current_winners" | jq -r '.[0]')
                local w2=$(echo "$current_winners" | jq -r '.[1]')
                parents="$TEMP_DIR/candidates/candidate-${w1}.json,$TEMP_DIR/candidates/candidate-${w2}.json"
            else
                strategy="mutation"
            fi
        fi

        # Generate candidates
        local candidates=$(generate_candidates "$round" "$strategy" "$parents")
        echo "$candidates" > "$TEMP_DIR/rounds/round-${round}-candidates.json"

        # Evaluate candidates in parallel
        evaluate_candidates_parallel "$candidates"

        # Collect results
        local results=$(collect_results)
        echo "$results" > "$TEMP_DIR/rounds/round-${round}-results.json"

        # Run tournament selection
        local selection=$(run_tournament "$TEMP_DIR/rounds/round-${round}-results.json")
        echo "$selection" > "$TEMP_DIR/rounds/round-${round}-selection.json"

        # Extract winners
        current_winners=$(echo "$selection" | jq -r '.winners')
        local best_score=$(echo "$selection" | jq -r '.rankings[0].score // 0')
        local best_id=$(echo "$selection" | jq -r '.rankings[0].id')
        local improvement=$(echo "$selection" | jq -r '.summary.bestImprovement // 0')

        log_success "Round $round winner: $best_id (score: $best_score, improvement: $improvement)"

        # Update round history
        round_history="${round_history}
### Round $round

- Strategy: $strategy
- Winner: $best_id
- Score: $best_score
- Improvement: $improvement
"

        # Check convergence
        local converged=$(check_convergence "$history_file" "$best_score")

        if [[ "$converged" == "true" ]]; then
            log_warn "Convergence detected (improvement < $CONVERGENCE_THRESHOLD for $CONVERGENCE_WINDOW rounds)"
            break
        fi

        echo ""
    done

    # Final results
    echo ""
    log_info "========== Search Complete =========="

    local final_winner=$(echo "$current_winners" | jq -r '.[0]')
    local final_config="$TEMP_DIR/candidates/candidate-${final_winner}.json"

    if [[ -f "$final_config" ]]; then
        log_success "Final winner: $final_winner"
        log_info "Config: $final_config"

        if ! $DRY_RUN; then
            # Copy winning config to output
            local output_config="$CONFIG_DIR/ensemble-winner-${timestamp}.json"
            cp "$final_config" "$output_config"
            log_success "Winner config saved to: $output_config"
        fi
    fi

    # Generate log
    local final_metrics=""
    if [[ -f "$TEMP_DIR/rounds/round-${MAX_ROUNDS}-results.json" ]]; then
        final_metrics=$(cat "$TEMP_DIR/rounds/round-${MAX_ROUNDS}-results.json")
    elif [[ -f "$TEMP_DIR/rounds/round-$((round))-results.json" ]]; then
        final_metrics=$(cat "$TEMP_DIR/rounds/round-$((round))-results.json")
    fi

    if ! $DRY_RUN; then
        generate_log "$timestamp" "$(echo "$current_winners" | jq -r '.')" "$round_history" "$final_metrics"
    fi

    echo ""
    log_success "Ensemble search complete!"
}

main "$@"
