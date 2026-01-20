---
epic_id: 2026-01-19-v55-pattern-expressiveness
title: "V5.5 Pattern Expressiveness - Hill-Climbing Iteration Targets"
status: backlog
created_date: 2026-01-19
updated_date: 2026-01-19
branch: feature/hill-climbing-epic
tasks: [46, 47, 48, 49, 50, 65]
priority_order: [46, 49, 47, 48, 50, 65]
completed_tasks: []
---

# Epic: V5.5 Pattern Expressiveness

## Vision

Systematically improve DuoPulse pattern generation metrics through targeted bug fixes and feature enhancements. Each task in this epic represents a hypothesis about algorithm improvements, with quantified improvement estimates that will be validated through the `/iterate` command workflow.

## Context

These tasks were originally created 2026-01-08 for V5.5 pattern expressiveness improvements. They were revisited 2026-01-19 to:
1. Assess relevancy with current system state
2. Add quantified improvement estimates (for hill-climbing validation)
3. Group into epic for systematic iteration
4. Update `/iterate` command to use estimate → implement → measure → log workflow

This epic serves as the test bed for the new hill-climbing iteration process.

## Goals

1. **Fix critical bugs** - Inverted noise formula (Task 46)
2. **Improve AUX variation** - From 20% to 50% unique patterns (Task 49)
3. **Enhance expressiveness** - Velocity dynamics and ghost notes (Task 47)
4. **Refine variation quality** - Micro-displacement over rotation (Task 48)
5. **Add safety net** - Two-pass generation if needed (Task 50)
6. **Future enhancement** - Phrase-aware weights with 64-step (Task 65)

## Success Metrics

**Overall Targets** (from design docs):
- Pattern variation: >= 50% unique patterns
- Groove quality: >= 60% metric score
- AUX variation: >= 50% unique patterns (currently 20%)
- Stable zone consistency: Near-deterministic at SHAPE < 0.3

**Per-Task Estimated Impact**:
- Task 46 (Noise Fix): +10-20% pattern stability, +10-15% variation quality
- Task 49 (AUX Styles + Beat 1): +30-35% AUX variation, +15-40% beat 1 reliability
- Task 47 (Velocity Variation): +20-25% expressiveness, +15-20% groove feel
- Task 48 (Micro-Displacement): +10-15% pattern variation, +10-15% stable zone consistency
- Task 50 (Two-Pass): CONDITIONAL - +10-15% groove quality IF needed
- Task 65 (Phrase-Aware): FUTURE - +20-30% phrase quality (requires Task 56)

**Cumulative Expected Improvement**: 40-60% overall metric improvement after tasks 46, 49, 47, 48

---

## Implementation Order

### Phase 1: Critical Fixes (High Confidence, High Impact)

**Task 46: Noise Formula Fix** (1 hour)
- **Priority**: FIRST (bug fix, other tasks depend on this)
- **Relevance**: HIGH - Inverted formula actively harming stability
- **Estimated Impact**: +15-20% pattern stability, +10-15% variation quality
- **Confidence**: 90% - Known bug with clear fix
- **Dependencies**: None

**Task 49: AUX Style Zones + Beat 1 Enforcement** (3 hours)
- **Priority**: SECOND (dual high-impact improvements)
- **Relevance**: HIGH - AUX at 20% variation, beat 1 missing breaks DJ mixing
- **Estimated Impact**: +30-35% AUX variation, +15-40% beat 1 reliability
- **Confidence**: 85% - Clear problem with proven solution
- **Dependencies**: Task 46 (bug fix first)

### Phase 2: Expressiveness Enhancements (High Impact, Medium Risk)

**Task 47: Velocity Variation** (2-3 hours)
- **Priority**: THIRD (major expressiveness boost)
- **Relevance**: HIGH - Groove quality heavily influenced by dynamics
- **Estimated Impact**: +20-25% expressiveness, +15-20% groove feel
- **Confidence**: 75% - Depends on whether ghost notes feel musical
- **Dependencies**: Task 46 (bug fix first)

**Task 48: Micro-Displacement** (3-4 hours)
- **Priority**: FOURTH (refinement over current rotation)
- **Relevance**: MEDIUM - More subtle than rotation, unclear if better
- **Estimated Impact**: +10-15% pattern variation, +10-15% stable zone consistency
- **Confidence**: 60% - Unclear if displacement beats rotation for metrics
- **Dependencies**: Task 46 (bug fix first)

### Phase 3: Safety Net (Conditional Implementation)

**Task 50: Two-Pass Generation** (4-5 hours OR 0 if dropped)
- **Priority**: LAST (implement only if targets not met)
- **Relevance**: CONDITIONAL - Skip unless tasks 46-49 don't meet targets
- **Estimated Impact**: +10-15% groove quality IF implemented
- **Confidence**: 70% - Adds structure but may reduce variation
- **Dependencies**: Tasks 46, 47, 48, 49 complete + metrics evaluation
- **Decision Criteria**:
  - If pattern variation >= 50% after phase 2: DROP this task
  - If groove quality >= 60% after phase 2: DROP this task
  - Otherwise: IMPLEMENT

### Phase 4: Future Enhancement (Deferred)

**Task 65: Phrase-Aware Weight Modulation** (4-6 hours)
- **Priority**: DEFERRED (requires prerequisites)
- **Relevance**: MEDIUM-LOW now, MEDIUM after 64-step grid
- **Estimated Impact**: +20-30% phrase quality (AFTER Task 56 complete)
- **Confidence**: 60% - Depends on API design and prerequisites
- **Dependencies**: Task 56 (weight blending), Task 55 (iteration system), Task 53 (64-step grid)
- **Status**: Move to backlog until prerequisites complete

---

## Iteration Workflow Integration

Each task in this epic will use the new `/iterate` command workflow:

### 1. Estimate Phase
```bash
# Before implementation, document predictions
- Which metrics will improve?
- By how much? (percentage estimates)
- What risks could prevent improvement?
- Confidence level in estimates
```

### 2. Implement Phase
```bash
# Standard SDD implementation
- Update code per task specification
- Run unit tests
- Build firmware
- Generate test patterns
```

### 3. Measure Phase
```bash
# Pentagon metrics evaluation
npm run evaluate
# Compare before/after
# Document actual improvements
```

### 4. Log Phase
```bash
# Record findings in iteration log
- Actual vs predicted improvement
- What worked / what didn't
- Lessons for future estimates
- Update epic with results
```

This workflow helps calibrate our estimation accuracy over time, improving future iteration planning.

---

## Risk Assessment

### High-Confidence Tasks (Tasks 46, 49)
- Clear bugs with known fixes
- Minimal implementation risk
- High expected impact
- **Recommendation**: Implement immediately

### Medium-Confidence Tasks (Tasks 47, 48)
- Well-designed but untested in practice
- May need tuning after implementation
- **Recommendation**: Implement with measurement checkpoints

### Low-Confidence Tasks (Task 50)
- Conditional on other tasks' results
- Adds complexity
- **Recommendation**: Measure first, implement only if needed

### Deferred Tasks (Task 65)
- Requires prerequisite infrastructure
- API changes needed
- **Recommendation**: Revisit after Task 56 complete

---

## Success Criteria for Epic Completion

**Minimum Success** (Phase 1 + 2 complete):
- [ ] Task 46 complete: Noise formula fixed
- [ ] Task 49 complete: AUX variation >= 50%, beat 1 >= 95% present
- [ ] Task 47 complete: Velocity range expanded, ghost notes functional
- [ ] Task 48 complete: Micro-displacement replaces rotation
- [ ] Pattern variation >= 50%
- [ ] Groove quality >= 60%

**Full Success** (All applicable tasks):
- [ ] All Phase 1 + 2 tasks complete
- [ ] Task 50 evaluated (implemented OR dropped with justification)
- [ ] Cumulative improvement >= 40% on Pentagon metrics
- [ ] No regressions introduced
- [ ] All tests passing
- [ ] Iteration log documents estimate accuracy

**Future Success** (Phase 4):
- [ ] Task 65 implemented after prerequisites
- [ ] Phrase structure quality improved
- [ ] 64-step grid synergy realized

---

## Timeline Estimate

**Phase 1** (Tasks 46, 49): 4 hours
**Phase 2** (Tasks 47, 48): 5-7 hours
**Phase 3** (Task 50 evaluation): 0.5 hours measure + (0 OR 4-5 hours implement)

**Total**: 9.5-16.5 hours depending on Task 50 decision

**Recommended Schedule**:
- Week 1: Phase 1 (critical fixes)
- Week 2: Phase 2 (expressiveness)
- Week 3: Phase 3 evaluation + Task 50 if needed

---

## Learning Outcomes

This epic serves dual purpose:
1. **Immediate**: Improve pattern generation metrics
2. **Strategic**: Validate hill-climbing iteration workflow

**Key Questions to Answer**:
- How accurate are our improvement estimates?
- Which types of changes yield highest ROI?
- Can we predict metric impact before implementing?
- What's the optimal order for improvements?

**Lessons will inform**:
- Future iteration planning
- Estimate calibration
- Risk assessment processes
- Task prioritization heuristics

---

## Notes

- This epic was created retrospectively from existing backlog tasks
- Estimates added 2026-01-19 to support new `/iterate` workflow
- Tasks 46-50 originally created 2026-01-08 during V5.5 design
- Task 65 created 2026-01-18 during Task 56 design review
- Epic represents ~10-16 hours of implementation work
- Expected cumulative improvement: 40-60% on key metrics
