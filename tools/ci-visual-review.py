"""Opt-in small UI render transport when artifact storage is inaccessible.
Only public GUI pixels, never tokens or build environment data, are emitted.
CI runs this only for commits explicitly marked [visual-review].
"""
import base64
import subprocess
import sys
from pathlib import Path
if sys.argv[1] == 'prepare':
    subprocess.run(['sips', '-s', 'format', 'jpeg', '-s', 'formatOptions', '40',
                    '--resampleWidth', '900', 'build/editor-snapshot.png',
                    '--out', 'build/editor-review.jpg'], check=True)
else:
    part = int(sys.argv[1])
    data = base64.b64encode(Path('build/editor-review.jpg').read_bytes()).decode()
    chunks = [data[i:i+3900] for i in range(0, len(data), 3900)]
    if len(chunks) > 40:
        raise RuntimeError('Review thumbnail exceeds annotation budget')
    for i in range(part*10, min((part+1)*10, len(chunks))):
        print(f'::notice title=UI_REVIEW_{i:03d}::{chunks[i]}')
