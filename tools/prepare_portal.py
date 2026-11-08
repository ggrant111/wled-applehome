"""PlatformIO pre-build: gzip web/portal.html into a tiny PROGMEM header.

Generate on each PlatformIO build; no runtime filesystem or SPIFFS needed.
"""
from pathlib import Path
import gzip

Import("env")  # SCons injected PlatformIO environment
root = Path(env["PROJECT_DIR"])
source = root / "web" / "portal.html"
target = root / "include" / "PortalGzip.generated.h"
raw = source.read_bytes()
compressed = gzip.compress(raw, compresslevel=9, mtime=0)
chunks = []
for i in range(0, len(compressed), 20):
    chunks.append("  " + ", ".join("0x%02x" % b for b in compressed[i:i+20]) + ",")
contents = (
    "#pragma once\n"
    "#include <Arduino.h>\n"
    "// Build-generated from web/portal.html; do not edit by hand.\n"
    "static const unsigned char kPortalGzip[] PROGMEM = {\n"
    + "\n".join(chunks)
    + "\n};\n"
    + "static constexpr size_t kPortalGzipLength = sizeof(kPortalGzip);\n"
)
target.parent.mkdir(parents=True, exist_ok=True)
if not target.exists() or target.read_text() != contents:
    target.write_text(contents)
print(f"[portal] HTML {len(raw):,} bytes -> gzip {len(compressed):,} bytes")
