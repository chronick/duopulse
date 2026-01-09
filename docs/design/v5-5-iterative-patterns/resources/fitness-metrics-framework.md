# Pattern Fitness Metrics Framework

A quantifiable scoring system for evaluating drum pattern generation quality, suitable for RL/hill-climbing optimization.

## Problem Statement

Current v5 statistics (from `docs/patterns.summary.txt`):
- **Overall variation**: 30.7% (FAIL) - too many identical patterns
- V1 (Anchor/Kick): 30.4% variation (36/116 unique)
- V2 (Shimmer/Hi-hat): 41.7% variation (49/116 unique)
- AUX: 20.0% variation (24/116 unique)

Named presets: 64.4% variation (better, but still not optimal)

**Goal**: Maximize expressiveness (unique patterns) while maintaining musical coherence per genre.

---

## Composite Fitness Score

```
FITNESS = w1*UNIQUENESS + w2*COHERENCE + w3*GENRE_AUTHENTICITY + w4*PARAMETER_RESPONSIVENESS

Default weights:
  w1 = 0.35  (uniqueness)
  w2 = 0.25  (coherence)
  w3 = 0.25  (genre authenticity)
  w4 = 0.15  (parameter responsiveness)
```

---

## Metric 1: UNIQUENESS (Pattern Variation)

Measures how many distinct patterns are generated across the parameter space.

### 1.1 Inter-Pattern Distance

For any two patterns P1, P2:
```
distance(P1, P2) = hamming_distance(P1.mask, P2.mask) / pattern_length
```

For a set of N patterns:
```
avg_distance = sum(distance(Pi, Pj)) / (N * (N-1) / 2)  for all i < j
```

**Target**: avg_distance >= 0.3 (30% of steps differ on average)

### 1.2 Uniqueness Ratio

```
uniqueness_ratio = unique_patterns / total_patterns
```

**Current**: 30.4% (anchor), 41.7% (shimmer), 20.0% (aux)
**Target**: >= 70% for all voices

### 1.3 Entropy of Step Activation

For each step position, compute activation probability across all patterns:
```
p[step] = hits_on_step / total_patterns
entropy[step] = -p * log2(p) - (1-p) * log2(1-p)  // binary entropy
total_entropy = sum(entropy) / pattern_length
```

**Target**: total_entropy >= 0.7 (high uncertainty in activation)

**Current issue**: Step 0 has 97% V1 activation = very low entropy on beat 1.

### 1.4 Cluster Analysis

Count distinct pattern clusters using simple threshold:
```
cluster_count = count patterns with no neighbor within distance 0.15
```

**Target**: cluster_count >= 10 for parameter sweeps

### UNIQUENESS Score Formula

```python
def uniqueness_score(patterns):
    avg_dist = compute_avg_distance(patterns)
    unique_ratio = len(set(patterns)) / len(patterns)
    entropy = compute_step_entropy(patterns)

    dist_score = min(1.0, avg_dist / 0.4)         # 0.4 is "perfect"
    ratio_score = min(1.0, unique_ratio / 0.8)    # 80% unique is "perfect"
    entropy_score = min(1.0, entropy / 0.8)       # 0.8 entropy is "perfect"

    return 0.4 * dist_score + 0.35 * ratio_score + 0.25 * entropy_score
```

---

## Metric 2: COHERENCE (Musical Validity)

Measures whether patterns are musically sensible, not just random.

### 2.1 Metric Weight Correlation

Compute correlation between step activation probability and metric weight:
```
metric_correlation = pearson(activation_prob, metric_weight)
```

For stable patterns (low SHAPE): expect positive correlation (downbeats more likely)
For syncopated patterns: expect lower/negative correlation

**Target by SHAPE zone**:
- SHAPE 0-30%: correlation >= 0.4
- SHAPE 30-70%: correlation in [-0.2, 0.4]
- SHAPE 70-100%: correlation < 0.3 (more random)

### 2.2 Density Consistency

```
expected_density = energy * pattern_length * density_factor
actual_density = popcount(pattern_mask)
density_error = |actual - expected| / expected
```

**Target**: density_error < 0.2 (within 20% of expected)

### 2.3 Gap Distribution

Measure gap lengths between hits:
```
gaps = [consecutive_zeros_count for each gap]
gap_variance = variance(gaps)
max_gap = max(gaps)
```

**Targets**:
- Low ENERGY: larger gaps OK (max_gap up to pattern_length/2)
- High ENERGY: max_gap <= pattern_length/4
- Gap variance should decrease with ENERGY

### 2.4 Voice Complement Quality

For shimmer (gap-filling):
```
anchor_gaps = find_gaps(anchor_mask)
shimmer_in_gaps = popcount(shimmer_mask & ~anchor_mask) / popcount(shimmer_mask)
```

**Target**: shimmer_in_gaps >= 0.8 (80% of shimmer hits are in anchor gaps)

### COHERENCE Score Formula

```python
def coherence_score(pattern, params):
    # Metric correlation check
    corr = compute_metric_correlation(pattern)
    corr_target = get_correlation_target(params.shape)
    corr_score = 1.0 - abs(corr - corr_target) / 1.0

    # Density check
    density_err = compute_density_error(pattern, params.energy)
    density_score = max(0, 1.0 - density_err / 0.3)

    # Gap distribution
    gap_score = compute_gap_score(pattern, params.energy)

    # Voice complement
    complement_score = compute_complement_quality(anchor, shimmer)

    return 0.25 * corr_score + 0.25 * density_score + 0.25 * gap_score + 0.25 * complement_score
```

---

## Metric 3: GENRE_AUTHENTICITY

Measures how well patterns match genre-specific expectations.

### 3.1 Genre Pattern Templates

Define expected step activation ranges per genre:

```cpp
struct GenreExpectation {
    float step_probability[32];  // expected prob for each step
    float tolerance;              // how much deviation is OK
    float kick_downbeat_prob;     // P(kick on step 0)
    float hat_offbeat_prob;       // P(hat on offbeats)
    float snare_backbeat_prob;    // P(snare on 2&4)
};
```

### 3.2 Genre Profiles

**Four on Floor / Techno**:
```
steps 0,4,8,12,16,20,24,28: kick_prob = 0.9
step 0: MUST have kick (prob > 0.95)
steps 8,24: snare_prob > 0.7
offbeats: hat_prob > 0.6
```

**House**:
```
steps 0,4,8,12,16,20,24,28: kick_prob = 0.85
offbeats (1,3,5,7,...): open_hat_prob > 0.5
steps 4,12,20,28: clap_prob > 0.6
```

**Breakbeat**:
```
step 0: kick_prob = 0.85
step 4: kick_prob < 0.5 (displaced)
step 8: snare_prob < 0.7 (displaced)
steps 7,15,23,31: accent positions
```

**IDM Complexity**:
```
Irregular groupings expected
Low correlation with metric grid
High entropy required
```

### 3.3 Genre Authenticity Computation

```python
def genre_authenticity(pattern, genre):
    expectations = GENRE_PROFILES[genre]
    score = 0.0

    for step in range(pattern_length):
        expected = expectations.step_probability[step]
        actual = 1.0 if pattern.has_hit(step) else 0.0
        error = abs(expected - actual) * expectations.weight[step]
        score += (1.0 - error)

    return score / pattern_length
```

### 3.4 Named Preset Validation

Each named preset should score highly against its declared genre:
```
Four on Floor -> TECHNO profile -> score >= 0.85
House Groove -> HOUSE profile -> score >= 0.80
IDM Chaos -> IDM profile -> score >= 0.70 (lower because chaos is expected)
```

---

## Metric 4: PARAMETER_RESPONSIVENESS

Measures how sensitively patterns change with control movements.

### 4.1 Sensitivity Gradient

For small parameter changes, pattern should change proportionally:
```
delta_pattern = hamming_distance(P(param), P(param + epsilon))
expected_delta = f(epsilon)  // should scale with epsilon
responsiveness = delta_pattern / expected_delta
```

**Target**: responsiveness in [0.5, 2.0] (not too sensitive, not too sluggish)

### 4.2 Monotonicity Check

Some parameters should have monotonic effects:
- ENERGY increasing -> density should never decrease significantly
- SHAPE increasing -> metric correlation should trend downward

```python
def monotonicity_score(patterns_by_param_value):
    violations = 0
    for i in range(len(patterns) - 1):
        if not expected_direction(patterns[i], patterns[i+1]):
            violations += 1
    return 1.0 - violations / (len(patterns) - 1)
```

### 4.3 Dead Zone Detection

Identify parameter ranges where pattern doesn't change:
```
dead_zones = ranges where 10%+ parameter sweep produces identical output
```

**Target**: dead_zones should cover < 10% of total parameter range

### 4.4 Full Parameter Space Coverage

Sweep all parameters and check that the output space is well-covered:
```
coverage = unique_patterns_from_full_sweep / theoretical_max_patterns
```

**Target**: coverage >= 0.60

---

## Implementation: Fitness Evaluator

```cpp
struct FitnessResult {
    float uniqueness;           // [0, 1]
    float coherence;            // [0, 1]
    float genre_authenticity;   // [0, 1]
    float responsiveness;       // [0, 1]
    float composite;            // weighted combination

    // Diagnostic breakdowns
    float avg_distance;
    float unique_ratio;
    float entropy;
    float metric_correlation;
    float density_error;
    float complement_quality;
};

FitnessResult EvaluateFitness(
    const std::vector<PatternResult>& patterns,
    const std::vector<PatternParams>& params,
    Genre genre
);
```

---

## Optimization Targets

### Minimum Viable (Current baseline to beat)
- Uniqueness >= 0.35 (current ~0.31)
- Coherence >= 0.60
- Genre authenticity >= 0.50
- Composite >= 0.50

### Good
- Uniqueness >= 0.50
- Coherence >= 0.70
- Genre authenticity >= 0.65
- Composite >= 0.65

### Excellent
- Uniqueness >= 0.70
- Coherence >= 0.80
- Genre authenticity >= 0.75
- Composite >= 0.75

### Perfect (theoretical ceiling)
- Uniqueness >= 0.85
- Coherence >= 0.85
- Genre authenticity >= 0.85
- Composite >= 0.85

---

## Testing Protocol

### Test Suite 1: Default Seed Sweep

Generate 116 patterns matching current visualization:
- All named presets
- Parameter sweeps (SHAPE, ENERGY, AXIS_X, AXIS_Y, DRIFT, ACCENT)
- Matrix combinations

**Success criteria**: Composite score > 0.65

### Test Suite 2: Seed Variation

Same parameters, 100 different seeds:
- Each seed should produce meaningfully different output
- Uniqueness across seeds should be > 0.90

### Test Suite 3: Genre Validation

For each named preset:
- Generate pattern
- Score against declared genre profile
- Must score > 0.70 for genre match

### Test Suite 4: Edge Cases

- ENERGY=0: Minimal but valid pattern
- ENERGY=1: Dense but not clipping
- SHAPE=0: Stable, predictable
- SHAPE=1: Chaotic but still rhythmic
- All parameters at 0.5: Neutral, balanced

---

## CLI Tool Integration

Extend `pattern_viz` tool to output fitness scores:

```bash
# Generate patterns and compute fitness
./pattern_viz --fitness --output-json metrics.json

# Output format
{
  "summary": {
    "composite": 0.52,
    "uniqueness": 0.31,
    "coherence": 0.68,
    "genre_authenticity": 0.55,
    "responsiveness": 0.45
  },
  "per_voice": {
    "anchor": { "unique_ratio": 0.304, "entropy": 0.45, ... },
    "shimmer": { "unique_ratio": 0.417, "entropy": 0.58, ... },
    "aux": { "unique_ratio": 0.200, "entropy": 0.32, ... }
  },
  "issues": [
    "Step 0 anchor activation too predictable (97%)",
    "AUX patterns have low entropy",
    "AXIS_Y sweep shows dead zone at [0.3, 0.5]"
  ]
}
```

---

## Next Steps

1. Implement FitnessEvaluator class
2. Run baseline evaluation on current algorithm
3. Identify specific weak points from diagnostic data
4. Design algorithm improvements targeting lowest-scoring metrics
5. Iterate until composite > 0.70
