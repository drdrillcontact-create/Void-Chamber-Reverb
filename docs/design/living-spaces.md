# Living Spaces preview

## Selected mapping

- Void: inward spiral/gravity field
- Hall: architectural light, side-orb glows and floor rings
- Plate: thin plasma ribbons and reactive glass rim
- Cavern: expanding elliptical floor ripples
- Ambient: layered cyan/violet aurora curtains

Procedural JUCE paths overlay the clean background; no screenshots replace live controls. Motion weights crossfade at 30 Hz. Input peak starts the response; pre-Mix wet RMS sustains it. Motion stops in silence. The visible MOTION ON/OFF button and M key toggle rendering. State is editor-local, not a new automatable sound parameter. No allocation, UI calls or animation work occurs on the audio thread. CPU/GPU performance still needs checking in real hosts.

## DSP changes

Three non-regenerative allpasses on each wet output increase temporal density, with higher diffusion for Plate. A 65 Hz wet-input highpass reduces low-end accumulation without filtering dry audio. Early reflections now use the per-space cutoff already specified in the profiles (previously unused). Modulation depth is sample-rate compensated and its range reduced for subtler pitch motion. Existing 8-line feedback topology and parameter IDs are retained. This is an incremental refinement, not a new reverb engine or a promise of subjective sound quality.

Wet RMS is published once per block with relaxed atomic access and remains available at Mix=0. Tests cover this at 44.1/48/96 kHz in addition to existing impulse, RT60 and space differentiation checks. Listening comparisons and DAW CPU measurements remain necessary.

## Art correction

Cavern and Ambient use a tighter interior crop to remove neighbouring medallion edges baked into the concept art. OrbButton supplies the live circular rim; `tools/prepare-smoked-glass.py` reproduces the crop.
