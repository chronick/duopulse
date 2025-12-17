# Comparison: Current vs. Two-Track Dual Design

## Current Implementation (Opinionated Sequencer)

### Approach
- Ported from 3-trigger system (kick, snare, hihat)
- Maps 3 internal voices to 2 outputs via "Voice Emphasis"
- Uses Grids pattern data (25 nodes, 3 parts per pattern)

### Control Layout
- **Performance Mode**: 4 controls (Low Density, High Density, Low Variation, High Variation)
- **Config Mode**: 4 controls (Style, Length, Emphasis, Tempo)
- **Total**: 8 controls + 8 CV inputs = 16 parameters

### Limitations
1. **Voice Mapping Confusion**: Trying to map 3 voices (kick, snare, hihat) to 2 outputs creates unpredictable routing
2. **Limited Expressiveness**: Only 2 controls per track (Density + Variation)
3. **Pattern Dependency**: Relies on pre-computed Grids patterns, limiting flexibility
4. **No Timing Control**: Cannot adjust micro-timing or create polyrhythmic feels
5. **No Dynamic Range Control**: Velocity range is fixed
6. **Predictability Issues**: Difficult to predict overall beat due to 3→2 voice mapping

## Two-Track Dual Design

### Approach
- **Embraces 2-track constraint** as creative strength
- **Dual-track rhythm engine** optimized for two-channel output
- **Algorithmic pattern generation** instead of pre-computed patterns

### Control Layout
- **Performance Mode**: 8 controls (4 base + 4 shift)
  - Base: Density ×2, Variation ×2
  - Shift: Velocity Range ×2, Timing Offset ×2
- **Config Mode**: 8 controls (4 base + 4 shift)
  - Base: Style, Length, Track Relationship, Tempo
  - Shift: Swing, Accent Pattern, Fill Probability, Gate Length
- **Total**: 16 controls + 16 CV inputs = 32 parameters

### Advantages

#### 1. **Predictable and Intuitive**
- Two tracks = two outputs, no mapping confusion
- Clear mental model: Track 1 (Low) + Track 2 (Mid/High)
- Each track independently controllable

#### 2. **Deep Expressive Control**
- **4 dimensions per track** in Performance Mode:
  - Density (hit frequency)
  - Variation (chaos/dynamics)
  - Velocity Range (dynamic range)
  - Timing Offset (micro-timing)
- **8 global dimensions** in Config Mode:
  - Style, Length, Relationship, Tempo
  - Swing, Accents, Fills, Gate Length

#### 3. **Creative Flexibility**
- **Track Relationship** control: Complementary, Independent, or Contrasting
- **Timing Offset**: Create polyrhythmic feels, phasing effects
- **Velocity Range**: Shape dynamics independently from pattern
- **Algorithmic Patterns**: Infinite variations, smooth interpolation

#### 4. **Performance-Optimized**
- **Quick access** to core controls (Density/Variation)
- **Shift access** to expressive controls (Velocity/Timing)
- **Config access** to character controls (Style/Relationship)
- **Shift Config** for refinement (Swing/Accents/Fills)

#### 5. **No Pattern Limitations**
- Not constrained by pre-computed Grids patterns
- Can generate patterns optimized for 2-track output
- Smooth parameter interpolation
- Real-time pattern evolution

## Feature Comparison

| Feature | Current | Two-Track Dual |
|---------|---------|----------------|
| **Tracks** | 3 voices → 2 outputs | 2 tracks → 2 outputs |
| **Pattern Source** | Pre-computed Grids data | Algorithmic generation |
| **Controls per Track** | 2 (Density, Variation) | 4 (Density, Variation, Velocity Range, Timing Offset) |
| **Timing Control** | ❌ None | ✅ Timing Offset per track |
| **Dynamic Range Control** | ❌ Fixed | ✅ Velocity Range per track |
| **Track Interaction** | ❌ Fixed routing | ✅ Track Relationship control |
| **Swing** | ❌ None | ✅ Configurable swing |
| **Accent Control** | ❌ Fixed | ✅ Accent Pattern control |
| **Fill Control** | ❌ Variation-based only | ✅ Fill Probability control |
| **Gate Length** | ❌ Fixed | ✅ Configurable |
| **Total Parameters** | 16 | 32 |
| **Predictability** | ⚠️ Low (3→2 mapping) | ✅ High (2→2 mapping) |

## Use Case Comparison

### Current: Trying to Play "Drum Patterns"
- User thinks: "I want kick, snare, hihat"
- System: Maps 3 voices to 2 outputs unpredictably
- Result: Confusing, hard to predict

### Two-Track Dual: Playing "Rhythmic Tracks"
- User thinks: "I want a driving low track and a complementary high track"
- System: Two independent, deeply controllable tracks
- Result: Predictable, expressive, creative

## Migration Path

### What Stays the Same
- Hardware layout (4 knobs + 4 CV inputs + button + switch)
- Soft takeover system
- External clock support
- Reset trigger
- Visual feedback (LED/CV Out 2)

### What Changes
- **Control mapping**: Shift button now accesses secondary controls
- **Pattern generation**: Algorithmic instead of Grids data lookup
- **Parameter set**: Expanded from 8 to 16 controls per mode
- **Voice model**: 2 tracks instead of 3 voices

### Implementation Effort
- **Medium**: Requires new pattern generation algorithms
- **Pattern Generator**: Rewrite to support algorithmic generation
- **Sequencer**: Update to support new parameters (Timing Offset, Velocity Range, Track Relationship, etc.)
- **Main Loop**: Update control mapping to support shift button

## Recommendation

The **Two-Track Dual** design is recommended because:

1. **Solves the core problem**: Predictable 2-track output
2. **Maximizes expressiveness**: 32 parameters vs. 16
3. **Embraces constraint**: Works with hardware limitations, not against them
4. **Future-proof**: Algorithmic patterns allow infinite expansion
5. **Performance-friendly**: Quick access to core controls, deep access to expressive controls

The current implementation's main weakness is trying to map 3 voices to 2 outputs, which creates unpredictability. The Two-Track Dual design embraces the 2-track constraint and maximizes creative control within that constraint.

