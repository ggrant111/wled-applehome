"""Ensure the checked-in web HTML stays compressible and CSP-safe."""
import gzip
from pathlib import Path
path = Path("web/portal.html")
data = path.read_bytes()
compressed = gzip.compress(data, compresslevel=9, mtime=0)
assert gzip.decompress(compressed) == data
assert len(compressed) < len(data) * 0.55, "Portal no longer compresses enough for 4MB OTA budget"
assert b'let CSRF="";' in data, "Never embed per-boot secret in cached gzip asset"
assert b'__CSRF__' not in data, "No runtime CSRF substitution in compressed HTML"
print(f"Portal asset compressed: {len(data)} -> {len(compressed)} bytes")
