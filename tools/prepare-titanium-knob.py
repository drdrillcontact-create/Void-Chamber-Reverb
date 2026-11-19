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
for y in range(298, 405):
    for x in range(485, 540):
        dx, dy = x-512, y-520
        r = math.hypot(dx, dy)
        angle = math.atan2(dx, -dy)
        if abs(angle) >= 0.12 or not 115 < r < 222:
            continue
        samples = []
        for side in (-0.13, 0.13):
            sx = round(512 + r * math.sin(side))
            sy = round(520 - r * math.cos(side))
            samples.append(source.getpixel((sx, sy)))
        t = (angle + 0.13) / 0.26
        replacement = [a*(1-t)+b*t for a,b in zip(*samples)]
        weight = min(1.0, (0.12-abs(angle))/0.035, (r-115)/10, (222-r)/10)
        old = source.getpixel((x,y))
        pixels[x,y] = tuple(round(a*(1-weight)+b*weight) for a,b in zip(old,replacement))
# Only the face and knurled grip survive; all control state is drawn by JUCE.
result = clean.crop((262,270,762,770)).convert('RGBA')
p = result.load()
for y in range(500):
    for x in range(500):
        alpha = round(255 * max(0, min(1, 249.5-math.hypot(x-249.5,y-249.5))))
        p[x,y] = (*p[x,y][:3], alpha)
Path('Assets').mkdir(exist_ok=True)
result.save('Assets/titanium-knob-body.png')
