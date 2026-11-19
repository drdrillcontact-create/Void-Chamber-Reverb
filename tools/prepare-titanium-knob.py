"""One-off artwork preparation, not a build dependency. Requires Pillow.
Extract selected Titanium face; remove the baked pointer, scale, arc and label.
Usage: python tools/prepare-titanium-knob.py
"""
from pathlib import Path
import math
from PIL import Image
source = Image.open('docs/design/knob-02-titanium.png').convert('RGB')
clean = source.copy()
pixels = clean.load()
# Same-radius samples outside the pointer preserve the concentric machining.
# Feather the narrow patch into the surrounding sunburst face.
for y in range(299, 402):
    for x in range(496, 529):
        dx, dy = x-512, y-520
        r = math.hypot(dx, dy)
        angle = math.atan2(dx, -dy)
        sample_angle = angle + (0.15 if dx >= 0 else -0.15)
        sx = round(512 + r * math.sin(sample_angle))
        sy = round(520 - r * math.cos(sample_angle))
        weight = min(1.0, (x-496)/5, (528-x)/5, (y-299)/5, (401-y)/5)
        old, new = source.getpixel((x,y)), source.getpixel((sx,sy))
        pixels[x,y] = tuple(round(a*(1-weight)+b*weight) for a,b in zip(old,new))
# Only the face and knurled grip survive; all control state is drawn by JUCE.
result = clean.crop((262,270,762,770)).convert('RGBA')
p = result.load()
for y in range(500):
    for x in range(500):
        alpha = round(255 * max(0, min(1, 249.5-math.hypot(x-249.5,y-249.5))))
        p[x,y] = (*p[x,y][:3], alpha)
Path('Assets').mkdir(exist_ok=True)
result.save('Assets/titanium-knob-body.png')
