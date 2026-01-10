import os
import sys
import markdown
from jinja2 import Environment, FileSystemLoader
import shutil
from pathlib import Path

# Optional PDF support
try:
    from weasyprint import HTML
    HAS_WEASYPRINT = True
except (ImportError, OSError):
    HAS_WEASYPRINT = False
    print("Note: WeasyPrint not available, skipping PDF generation.")

# Paths
BASE_DIR = Path(__file__).parent
ROOT_DIR = BASE_DIR.parent.parent
DOCS_DIR = ROOT_DIR / "docs"
PUBLIC_DIR = DOCS_DIR / "public"
OUTPUT_DIR = BASE_DIR / "output"
TEMPLATE_DIR = BASE_DIR / "templates"
STATIC_DIR = BASE_DIR / "static"

# Source files to combine into the manual
SPEC_FILES = [
    DOCS_DIR / "specs" / "main.md",
]

def generate_manual():
    # Ensure output directory exists
    if OUTPUT_DIR.exists():
        shutil.rmtree(OUTPUT_DIR)
    OUTPUT_DIR.mkdir()

    # Read and combine all spec files
    md_content_parts = []
    for spec_file in SPEC_FILES:
        if not spec_file.exists():
            print(f"Warning: Spec file not found at {spec_file}, skipping...")
            continue
        
        with open(spec_file, "r", encoding="utf-8") as f:
            content = f.read()
            md_content_parts.append(content)
            print(f"Loaded: {spec_file.name}")
    
    if not md_content_parts:
        print("Error: No spec files found!")
        sys.exit(1)
    
    # Combine with page breaks between sections
    md_content = "\n\n---\n\n".join(md_content_parts)

    # Convert Markdown to HTML
    # Using extensions for tables, etc.
    html_content = markdown.markdown(
        md_content,
        extensions=['tables', 'fenced_code', 'toc']
    )

    # Prepare Jinja2 environment
    env = Environment(loader=FileSystemLoader(TEMPLATE_DIR))
    template = env.get_template("manual.html")

    # Image handling: 
    # WeasyPrint needs file:// paths or absolute paths for local images if not served.
    # We will copy images to output folder for the HTML version, 
    # and provide absolute paths for WeasyPrint if needed.
    
    # Copy public assets to output
    if PUBLIC_DIR.exists():
        shutil.copytree(PUBLIC_DIR, OUTPUT_DIR / "images")

    # Copy static assets (css)
    shutil.copytree(STATIC_DIR, OUTPUT_DIR / "static")

    # Data for template
    context = {
        "content": html_content,
        "title": "DuoPulse v5",
        "css_path": "static/style.css",
        # We might want to inject the cover image if it exists
        "cover_image": "images/patch_init_front_square.webp"
    }

    # Render HTML
    rendered_html = template.render(context)
    html_output_path = OUTPUT_DIR / "manual.html"
    
    with open(html_output_path, "w", encoding="utf-8") as f:
        f.write(rendered_html)
    
    print(f"Generated HTML at {html_output_path}")

    # Generate PDF (if WeasyPrint is available)
    if HAS_WEASYPRINT:
        pdf_output_path = OUTPUT_DIR / "manual.pdf"
        
        # WeasyPrint needs a base_url to find relative resources (images, css)
        # relative to the HTML file location.
        HTML(string=rendered_html, base_url=str(OUTPUT_DIR)).write_pdf(pdf_output_path)
        
        print(f"Generated PDF at {pdf_output_path}")
    else:
        print("PDF generation skipped (install WeasyPrint + system libraries for PDF support)")

if __name__ == "__main__":
    generate_manual()

