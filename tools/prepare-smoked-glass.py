"""Prepare static scene and icon art from the approved design. Requires Pillow.
No labels, controls, value arcs or graphs are baked into the backdrop.
"""
from PIL import Image
from pathlib import Path
# Standalone scene previously selected during the art-direction session.
scene=Image.open('docs/design/scene-03-liquid-mirror-sanctum.png').convert('RGB')
scene=scene.resize((1106,737),Image.Resampling.LANCZOS)
canvas=Image.new('RGB',(1536,1024),(3,6,10));canvas.paste(scene,(215,105))
p=canvas.load()
for y in range(105,842):
 for x in range(215,1321):
  # Seamless falloff into the UI underlay, leaving title and controls quiet.
  w=min(1.,(x-215)/70,(1320-x)/70,(y-105)/45,(841-y)/80)
  w=max(0,w)
  if y>520:w*=max(.16,1-(y-520)/220*.84)
  old=p[x,y];p[x,y]=tuple(round(c*w+b*(1-w)) for c,b in zip(old,(3,6,10)))
canvas.save('Assets/void-background.jpg',quality=96)
# Extract only the circular art from the approved integrated concept.
concept=Image.open('docs/design/composition-b-smoked-glass.png').convert('RGB')
boxes=[(42,793,140,891),(144,793,242,891),(242,793,340,891),(340,793,438,891),(438,793,536,891)]
for name,box in zip(['void','hall','plate','cavern','ambient'],boxes):
 concept.crop(box).resize((256,256),Image.Resampling.LANCZOS).save('Assets/space-'+name+'.jpg',quality=96)
