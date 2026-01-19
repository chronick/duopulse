---
id: 67
slug: dashboard-improvements
title: "Evaluation Dashboard UX Improvements"
status: completed
created_date: 2026-01-19
completed_date: 2026-01-19
branch: feature/dashboard-improvements
spec_refs: []
depends_on:
  - 54  # Fill gates in evals
  - 63  # Sensitivity analysis
---

# Task 67: Evaluation Dashboard UX Improvements

## Objective

Improve the usability and clarity of the pattern evaluation dashboard based on user feedback:
1. Add metrics progress timeline chart to overview page
2. Improve fill section with clearer parameter displays
3. Add fill state visual indicators to patterns
4. Add sensitivity-matrix generation to CI workflow

## Context

### User Feedback

From dashboard review on 2026-01-19:

1. **Timeline chart needed**: Want to see metrics progress over time with git SHA + timestamps since evaluations started
2. **Fill section unclear**: Not clear what parameters are being shown, want to adjust more than just energy
3. **Fill state visualization**: Fill state should show as a line (high/low indicator) on all patterns
4. **Missing sensitivity data**: Sensitivity tab empty because `make sensitivity-matrix` not running in CI

### Current State

- Dashboard has Overview, Presets, Sweeps, Seeds, Fills, and Sensitivity tabs
- Fill section only varies energy, other params fixed
- No historical view of metric improvements
- Sensitivity data not generated in CI

## Implementation

### 1. Metrics History Timeline Chart

**Created**: `scripts/extract-metrics-history.js`
- Extracts historical Pentagon metrics from baseline-v* tags
- Walks through `metrics/baseline.json` at each tag
- Generates `tools/evals/public/data/metrics-history.json` with timeline

**Created**: `tools/evals/public/js/timeline.js`
- SVG line chart showing overall alignment score over time
- Data points labeled with git tags/hashes
- Color-coded by alignment status (GOOD/FAIR/POOR)
- Summary stats: starting score, current score, change, data points

**Modified**: `tools/evals/public/app.js`
- Import and render timeline chart in overview view
- Load metrics-history.json (optional)
- Inject timeline styles

**Modified**: `Makefile`
- Added `metrics-history` target to run extraction script

**Modified**: `.github/workflows/pages.yml`
- Added "Extract metrics history" step before baseline update

### 2. Fill Section Improvements

**Modified**: `tools/evals/public/app.js` (renderFillsView)
- Added fill configuration section explaining what fills are
- Display base parameters (SHAPE, AXIS-X, AXIS-Y, DRIFT, ACCENT)
- Show which parameter is being swept (ENERGY)
- Added explanatory note about fill state indicators

**Modified**: `tools/evals/public/css/components.css`
- Added `.fill-config-section` styles
- Added `.fill-description`, `.fill-params`, `.param-row` styles
- Added `.fill-note` warning box styling

### 3. Fill State Indicators

**Modified**: `tools/evals/public/app.js` (renderFillPatternGrid)
- Added fill state indicator row above pattern rows
- Shows horizontal line across all steps when fill is active
- Visual distinction: active (orange glow) vs inactive (gray)

**Modified**: `tools/evals/public/css/components.css`
- Added `.fill-state-row` and `.fill-state-indicator` styles
- Active state: warning color with glow effect
- Inactive state: dim gray

### 4. Sensitivity Matrix in CI

**Modified**: `.github/workflows/pages.yml`
- Added "Generate sensitivity matrix" step after evaluations
- Runs `make sensitivity-matrix` before publishing site

### 5. Additional Fixes (Post-Review)

**Modified**: `tools/evals/public/app.js` (renderSensitivityView)
- Pass callback to renderSensitivityHeatmap to enable clickable cells
- Add event delegation for cell click handlers
- Show placeholder alert when clicking sensitivity cells (future: sweep visualization)

**Modified**: `tools/evals/public/js/pattern-grid.js`
- Added `renderFillStateRow()` function
- Added `showFillState` and `fillActive` options to `renderPatternGrid()`
- Fill state indicator now shown on all patterns (presets, sweeps, seeds)
- Default: inactive state (gray line) for normal patterns

**Modified**: `tools/evals/public/css/pattern-grid.css`
- Added `.fill-state-row`, `.fill-state-label`, `.fill-state-indicator` styles
- Active state: orange with glow effect
- Inactive state: gray, low opacity

## Files Changed

### Created
- `scripts/extract-metrics-history.js` - Extract metrics from baseline tags
- `tools/evals/public/js/timeline.js` - Timeline chart component
- `tools/evals/public/data/metrics-history.json` - Generated timeline data (not committed)

### Modified
- `.github/workflows/pages.yml` - Add sensitivity and timeline to CI
- `Makefile` - Add metrics-history target
- `tools/evals/public/app.js` - Timeline, fill improvements, sensitivity click handlers
- `tools/evals/public/css/components.css` - Timeline and fill styles
- `tools/evals/public/js/pattern-grid.js` - Add fill state indicator to all patterns
- `tools/evals/public/css/pattern-grid.css` - Fill state indicator styles

## Testing

```bash
# Generate metrics history locally
make metrics-history

# View generated data
cat tools/evals/public/data/metrics-history.json

# Serve dashboard locally
make evals-serve
# Open http://localhost:3000
# Navigate to Overview tab - should see timeline chart at top
# Navigate to Fills tab - should see improved parameter display and fill state indicator
# Navigate to Sensitivity tab - should have data after running `make sensitivity-matrix`
```

## Validation

- [x] Timeline chart displays on overview page with git tags and scores
- [x] Timeline shows improvement trend over baseline versions
- [x] Fill section shows all base parameters clearly
- [x] Fill section includes explanatory text about what's being swept
- [x] Fill patterns have fill state indicator row
- [x] Fill state indicator visually distinct (active = orange, inactive = gray)
- [x] Sensitivity matrix runs in CI workflow
- [x] Sensitivity cells are clickable (shows alert for now)
- [x] All pattern views show fill state indicator (presets, sweeps, seeds)
- [x] Fill state shows inactive (gray) on normal patterns
- [x] All existing dashboard functionality still works
- [x] No console errors in browser dev tools

## Notes

- Timeline data tracked via baseline tags (baseline-v1.0.0, v1.0.1, etc.)
- Currently only 3 baseline versions exist, chart will be more useful with more history
- Fill parameter sweeps could be expanded in future (multi-dimensional sweeps)
- Fill state indicator currently only shown in fill section, could add to other views

## Future Enhancements (Deferred)

1. **Interactive timeline**: Click data points to view pattern details at that version
2. **Multi-metric timeline**: Toggle between alignment, zone compliance, individual metrics
3. **Comparison mode**: Overlay multiple timelines (stable vs syncopated vs wild zones)
4. **Fill parameter grid**: UI to generate custom fill sweeps with different base params
5. **Fill state on all patterns**: Add fill indicator to presets/sweeps views
6. **Export timeline data**: Download CSV/JSON of historical metrics
