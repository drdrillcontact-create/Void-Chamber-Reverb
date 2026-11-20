# Approved direction: Smoked Glass + Titanium

The user chose the second integrated mockup, `composition-b-smoked-glass.png`,
with the previously approved Titanium knobs retained. This supersedes the
intermediate component-by-component art-direction choices.

## Implemented as real controls
- Layout measured against the 1536×1024 mockup: slim header, tall glass meters,
  separate stereo buttons, central dome, paired knob bays, three lower panels.
- Layered glass shading, metallic hairlines and panel edge reflections are JUCE
  vector graphics. Text, meters, peak/clip lamps, selector state and graphs are
  not baked into a screenshot.
- Embedded Cormorant Garamond title font (OFL license alongside the font), with
  Raleway labels and improved contrast. No host-system font dependency.
- The standalone Liquid Mirror scene supplies static scenery only. Five icon
  crops come from the selected integrated mockup. `tools/prepare-smoked-glass.py`
  regenerates these assets with Pillow; Pillow is not needed for plugin builds.
- Titanium knob artwork/automation remains intact. Four main knobs use cyan
  accents; Mix uses pink. All existing parameter IDs and DSP are unchanged.
- Meter ticks, fill and peak share a calibrated -60..0 dB mapping; numeric labels
  and clip indications remain live. Decay uses the actual attached RT60 value;
  its stylised envelope is illustrative, not a claimed measured analyser.
- Frequency Decay stays parameter-dependent analytical absorption, not a static
  reproduction of the mockup's decorative frequency curves.

## Verification and delivery
Snapshot tests cover both knob accents at min/mid/max, meter calibration,
Space Type and Mono/Stereo actions, preset overlay opening/closing, and rendering
at 1536×1024, 960×640, 1200×900 and 1800×900 (including forced off-ratio host sizes).
Commit marker `[visual-review]` opts into a reduced actual CI-render thumbnail
transport for restricted environments. Marker `[ui-preview]` publishes tested
Windows VST3, universal macOS AU and full-resolution PNGs only after both builds
and tests pass. Published previews are unsigned, not notarized final releases.
