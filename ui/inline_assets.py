import os
import re

DIST_DIR = "dist"
INDEX_HTML = os.path.join(DIST_DIR, "index.html")
OUTPUT_HTML = "../Source/ui.html"

def inline_assets():
    with open(INDEX_HTML, "r") as f:
        html = f.read()

    # Inline CSS
    css_match = re.search(r'<link rel="stylesheet"[^>]*href="([^"]+)"[^>]*>', html)
    if css_match:
        css_path = css_match.group(1)
        if css_path.startswith("/"):
            css_path = css_path[1:]
        
        full_css_path = os.path.join(DIST_DIR, css_path)
        print(f"Inlining CSS from {full_css_path}")
        with open(full_css_path, "r") as f:
            css_content = f.read()
        
        html = html.replace(css_match.group(0), f"<style>{css_content}</style>")

    # Inline JS
    js_match = re.search(r'<script type="module"[^>]*src="([^"]+)"[^>]*></script>', html)
    if js_match:
        js_path = js_match.group(1)
        if js_path.startswith("/"):
            js_path = js_path[1:]

        full_js_path = os.path.join(DIST_DIR, js_path)
        print(f"Inlining JS from {full_js_path}")
        with open(full_js_path, "r") as f:
            js_content = f.read()
        
        # Escape </script> to prevent premature HTML script tag closure
        js_content = js_content.replace("</script>", "<\\/script>")
            
        html = html.replace(js_match.group(0), "")
        
        inline_script = f'<script>{js_content}</script>'
        if "</body>" in html:
            html = html.replace("</body>", f"{inline_script}</body>")
        else:
            html += inline_script

    with open(OUTPUT_HTML, "w") as f:
        f.write(html)
    
    print(f"Successfully generated {OUTPUT_HTML}")

if __name__ == "__main__":
    inline_assets()
