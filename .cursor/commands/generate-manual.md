# Generate Module Manual

This command generates a HTML and PDF manual from the module specification.

## Prerequisites
- `uv` (Python package manager) must be installed.
- Python 3.9+
- System libraries for WeasyPrint (PDF generation):
  - macOS: `brew install pango libffi cairo gdk-pixbuf`
  - Linux (Debian/Ubuntu): `apt-get install python3-pip python3-cffi python3-brotli libpango-1.0-0 libpangoft2-1.0-0`

## Usage

Run the generation script using `uv`:

```bash
cd scripts/manual_generator
uv run generate.py
```

## Output
- HTML Manual: `scripts/manual_generator/output/manual.html`
- PDF Manual: `scripts/manual_generator/output/manual.pdf`

## Implementation Details
- Source Spec: `docs/specs/main.md`
- Media Source: `docs/public/`
- Generator Script: `scripts/manual_generator/generate.py`
- Configuration: `scripts/manual_generator/pyproject.toml`
