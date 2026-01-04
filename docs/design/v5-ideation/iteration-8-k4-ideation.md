# K4 Config Ideation

**Context**: With LENGTH removed, K4 in Config mode is free. What continuous parameter would be useful?

**Knob Pairing Reference**:
```
K1: ENERGY / CLOCK DIV   (density / rate)
K2: SHAPE / SWING        (regularity / groove)
K3: AXIS X / DRIFT       (beat position / evolution)
K4: AXIS Y / ???         (intricacy / ???)
```

AXIS Y controls Simple ↔ Complex. What config-level parameter relates to "intricacy" or "structure"?

---

## Category 1: AXIS-Related

### AXIS RANGE / SPREAD
**What it does**: Controls how much AXIS X/Y knobs affect the pattern.
- 0% = Narrow range (subtle changes when turning AXIS knobs)
- 100% = Full range (dramatic changes)

**Use case**: "I want fine control in this patch" vs "I want big sweeping changes"

**Pairing logic**: Both AXIS Y and AXIS RANGE are about "how much complexity is available"

**Verdict**: Interesting but might feel like a "preferences" setting rather than musical

---

### AXIS COUPLING
**What it does**: How X and Y interact with each other.
- 0% = Independent (X and Y are orthogonal)
- 50% = Slight coupling (moving X influences Y slightly)
- 100% = Fully coupled (X and Y move together, like a single diagonal axis)

**Use case**: Simplify from 2D to 1D navigation when patching a single CV to both

**Verdict**: Niche, might confuse more than help

---

## Category 2: Voice/Accent Related

### ACCENT (Velocity Depth)
**What it does**: How much velocity variation exists in the pattern.
- 0% = All hits same velocity (flat dynamics)
- 100% = Wide velocity range (dynamic, expressive)

**Use case**: "I want punchy consistent hits" vs "I want ghost notes and accents"

**Pairing logic**: Complex patterns (high AXIS Y) often benefit from dynamic accents. Simple patterns might want flat velocity.

**Note**: User said "PUNCH can be handled by VCAs" - but this is about *internal* velocity variation, not output level. VCAs can't add ghost notes.

**Verdict**: Strong candidate - directly affects musicality, "set and forget" caliber

---

### VOICE SPREAD
**What it does**: How different Voice 2 is from Voice 1.
- 0% = Similar (V2 closely mirrors V1's density/feel)
- 100% = Maximally complementary (V2 fills all gaps V1 leaves)

**Use case**: "I want two similar voices" vs "I want clear call/response"

**Pairing logic**: Relates to complexity - more spread = more complex interplay

**Note**: User said "voice coupling not important" in early feedback - might not want this resurfaced

**Verdict**: Risky given earlier feedback, but could work

---

## Category 3: Pattern Memory/Response

### CV RESPONSE (Slew)
**What it does**: How quickly pattern changes respond to CV/knob movement.
- 0% = Instant (pattern regenerates immediately on change)
- 100% = Slewed (pattern morphs gradually over several phrases)

**Use case**: "I want immediate response to CV" vs "I want smooth evolution"

**Pairing logic**: Relates to how "tight" or "loose" the module feels in a patch

**Verdict**: Interesting for integration but might be better as a compile-time choice

---

### PHRASE MEMORY
**What it does**: How much patterns "remember" vs regenerate.
- 0% = Always fresh (each phrase is regenerated from scratch)
- 100% = Sticky (patterns lock in and only change when you force reseed)

**Use case**: "I want constant evolution" vs "I want to find a good pattern and keep it"

**Pairing logic**: Structure/stability dimension - pairs with AXIS Y's "intricacy"

**Verdict**: Interesting concept but overlaps with DRIFT's purpose

---

## Category 4: Pattern Space Navigation

### SEED OFFSET (Pattern Browser)
**What it does**: Continuous offset to the pattern seed - smoothly morphs through variations.
- Like a "preset browser" knob
- Each position gives a different base pattern character
- AXIS X/Y still navigate within that character

**Use case**: "I want to explore different pattern families without reseeding"

**Pairing logic**: AXIS Y navigates within complexity; SEED OFFSET navigates between pattern families

**Interaction with DRIFT**: DRIFT adds phrase-to-phrase variation; SEED OFFSET changes the base character

**Verdict**: Very interesting - adds a new dimension of exploration

---

### COMPLEXITY BIAS
**What it does**: Shifts the entire AXIS Y range up or down.
- 0% = AXIS Y range is 0-50% complexity
- 50% = AXIS Y range is 25-75% complexity (centered)
- 100% = AXIS Y range is 50-100% complexity

**Use case**: "I want even my 'simple' patterns to have some complexity" or vice versa

**Pairing logic**: Direct modification of AXIS Y's operating range

**Verdict**: Useful but might be over-engineering

---

## Category 5: Output Characteristics

### GATE LENGTH
**What it does**: Duration of trigger outputs.
- 0% = Short triggers (1ms)
- 100% = Long gates (full step length)

**Use case**: Different drum modules prefer different trigger lengths

**Pairing logic**: Weak pairing with AXIS Y

**Verdict**: Practical but boring, feels like a calibration setting

---

### HUMANIZE
**What it does**: Micro-timing variation independent of SHAPE.
- 0% = Quantized to grid
- 100% = Significant timing drift

**Use case**: "I want euclidean patterns but with human feel"

**Note**: SHAPE already adds timing irregularity at high values. This would add it at low SHAPE too.

**Pairing logic**: Both HUMANIZE and AXIS Y affect pattern "feel"

**Verdict**: Overlaps with SHAPE, might be redundant

---

## Recommendation Ranking

| Option | Musical Value | Knob Pairing | Overlap Risk | Recommendation |
|--------|---------------|--------------|--------------|----------------|
| **ACCENT** | High | Good (intricacy ↔ dynamics) | Low | ⭐ Top pick |
| **SEED OFFSET** | High | Medium | Low | ⭐ Creative pick |
| **AXIS RANGE** | Medium | Good | Low | Worth considering |
| VOICE SPREAD | Medium | Medium | User said no | Risky |
| CV RESPONSE | Medium | Weak | Low | Niche |
| GATE LENGTH | Low | Weak | Low | Too boring |
| HUMANIZE | Medium | Medium | High (SHAPE) | Skip |
| COMPLEXITY BIAS | Low | Good | Medium | Over-engineered |

---

## Top Two Candidates

### Option 1: ACCENT (Velocity Depth)

```
K4 Config: ACCENT
- 0%: Flat dynamics (all hits equal velocity)
- 100%: Full dynamics (ghost notes to accents)

Knob Pairing:
K4 Perf: AXIS Y (Simple ↔ Complex patterns)
K4 Config: ACCENT (Flat ↔ Dynamic velocities)
Both about "intricacy/depth"
```

**Why it works**:
- Directly affects musicality
- "Set and forget" - you dial in how dynamic you want the module
- VCAs can't add ghost notes - this is internal character
- Natural pairing: complex patterns often want dynamic accents

---

### Option 2: SEED OFFSET (Pattern Browser)

```
K4 Config: SEED OFFSET
- Continuous morphing through pattern families
- Like a "character" or "flavor" selector
- Reseed gives random; SEED OFFSET gives controlled exploration

Knob Pairing:
K4 Perf: AXIS Y (navigate within current pattern)
K4 Config: SEED OFFSET (navigate between pattern families)
Both about "exploring pattern space"
```

**Why it works**:
- Adds a new creative dimension
- Makes pattern exploration more deliberate than random reseed
- "Set and forget" once you find a family you like
- Interesting interaction with DRIFT (offset changes base, drift adds variation)

---

## Hybrid Possibility

Could rename K3 Config to include both concepts:

```
K3: DRIFT + SEED behavior
- Low values: patterns drift subtly within same family
- High values: patterns drift and also shift through seed space
```

But this might be too complex. Probably better to pick one clear function for K4.
