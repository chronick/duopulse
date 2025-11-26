# Manual Generator

This tool generates PDF and HTML manuals for the DaisySP IDM Grids module from the markdown specification files.

## Prerequisites

### 1. Python Package Manager (uv)
This project uses [uv](https://github.com/astral-sh/uv) for Python dependency management.
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

### 2. System Dependencies (WeasyPrint)
The PDF generation relies on `WeasyPrint`, which requires system libraries for rendering.

**macOS (Homebrew):**
```bash
brew install pango libffi cairo gdk-pixbuf
```

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get install python3-pip python3-cffi python3-brotli libpango-1.0-0 libpangoft2-1.0-0
```

For other systems, see the [WeasyPrint installation guide](https://doc.courtbouillon.org/weasyprint/stable/first_steps.html#installation).

## Usage

You can use the provided helper script from the project root:

```bash
./scripts/generate_manual.sh
```

Or run it directly with `uv`:

```bash
cd scripts/manual_generator
uv run generate.py
```

## Output

Generated files are placed in the `output/` directory:
- `output/manual.html`
- `output/manual.pdf`

## Project Structure

- `generate.py`: Main Python script
- `templates/`: Jinja2 HTML templates
- `static/`: CSS and other static assets
- `pyproject.toml`: Python dependencies
- `output/`: Generated artifacts

