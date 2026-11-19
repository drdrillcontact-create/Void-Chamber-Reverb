"""Temporary public UI-only preview transport for artifact-blocked reviewers."""
import base64
import sys
from pathlib import Path
part = int(sys.argv[1])
data = base64.b64encode(Path('build/editor-review.jpg').read_bytes()).decode()
chunks = [data[i:i+3000] for i in range(0, len(data), 3000)]
for i in range(part * 10, min((part + 1) * 10, len(chunks))):
    print(f'::notice title=UI_REVIEW_{i:03d}::{chunks[i]}')
