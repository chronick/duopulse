---
id: 51
slug: github-pages
title: "GitHub Pages Site with Evals Integration"
status: completed
created_date: 2026-01-13
updated_date: 2026-01-13
branch: feat/github-pages
spec_refs: []
---

# Task 51: GitHub Pages Site with Evals Integration

## Objective

Create a proper GitHub Pages site for DuoPulse with a landing page that links to the existing Pattern Evaluation Dashboard, consolidating all web-based content under a single, professionally styled site.

## Background

The project currently has:
- An evals page (`tools/evals/public/`) that gets deployed to GitHub Pages
- A CI workflow (`pages.yml`) that deploys evals content

This task will:
1. Add a front page (index.html) as the landing page
2. Move evals to `/evals/` subpath
3. Update CI/CD to run tests on push/PR and deploy the complete site
4. Apply consistent Eurorack-themed styling

## Subtasks

### Landing Page
- [x] Create `site/index.html` with:
  - Project title: "DuoPulse v5"
  - Description: "2-Voice Algorithmic Percussion Sequencer for Eurorack"
  - Link to GitHub repository
  - Link to spec documentation (external or embedded)
  - Patch.Init module image/graphic (CSS-based panel visualization)
  - Navigation to evals page
  - Section explaining hill-climbing/evaluation methodology
- [x] Create `site/styles.css` with Eurorack-themed dark/industrial color scheme:
  - Dark background (#0d0d0d to #1a1a1a range)
  - Accent colors: modular synth inspired (LED blues, warm oranges, muted greens)
  - Industrial typography
  - Responsive design

### Site Structure
- [x] Create `site/` directory as the main deployment source
- [x] Move/copy evals content to `site/evals/` during build
- [x] Ensure navigation links work between pages:
  - Index -> Evals: `/evals/`
  - Evals -> Index: `../`

### CI/CD Workflow Updates
- [x] Update `.github/workflows/pages.yml` to:
  - Run tests on push to main
  - Run tests on PRs
  - Build the complete site (landing + evals)
  - Deploy to GitHub Pages on main branch push
- [x] Keep test.yml separate for general testing
- [x] Add path triggers for new site content

### Assets
- [x] Add Patch.Init module visualization (CSS-only panel graphic)
- [x] Ensure all assets are properly referenced

## Acceptance Criteria

- [x] Landing page displays at repository's GitHub Pages root URL
- [x] Evals dashboard accessible at `/evals/`
- [x] Navigation works bidirectionally between pages
- [x] CI runs tests on all PRs
- [x] Site deploys automatically on push to main
- [x] Dark, industrial color scheme consistent across all pages
- [x] Responsive design works on mobile
- [x] No broken links or missing assets

## Implementation Notes

### Directory Structure
```
site/
  index.html        # Landing page
  styles.css        # Main site styles
  evals/            # Copied from tools/evals/public during build
    index.html
    app.js
    styles.css
    data/
```

### Color Palette (used)
- Background: #0d0d0d, #151515, #1a1a1a
- Text: #e0e0e0, #a0a0a0
- Accent Primary: #4a90d9 (LED blue)
- Accent Secondary: #ff6b35 (warm orange - matches V1)
- Accent Tertiary: #4ecdc4 (teal - matches V2)
- Border/Divider: #333333

### GitHub Pages Deployment
- Root URL: `https://<org>.github.io/duopulse/`
- Evals URL: `https://<org>.github.io/duopulse/evals/`

### Files Modified
- `.github/workflows/pages.yml` - Added test job, site assembly, path triggers

### Files Created
- `site/index.html` - Landing page with module info and evals explanation
- `site/styles.css` - Eurorack-themed styles
- `tools/evals/public/css/layout.css` - Added back-link styles
- `tools/evals/public/index.html` - Added back-link to home
