# Selected Titanium rotary controls

The user selected `knob-02-titanium.png`. `tools/prepare-titanium-knob.py`
extracts its brushed-metal face and knurled grip, removes the baked indicator,
and masks away the concept background, scale, text and pink arc. Pillow is
needed only to regenerate this source artwork, not to build the plugin.

JUCE paints the stationary bezel and calibration ticks, parameter-dependent
pink/cyan arc, and moving warm-white inset indicator independently. The cached
500 px face is shared by Size, Pre Delay, Damping, Modulation and Mix; existing
slider attachments, gestures and automation are unchanged. Decay remains the
large central arc. UI scaling continues to use the common editor transform.

The snapshot test checks the two accents at min/mid/max, emits a six-knob PNG,
and renders the full editor at four sizes. A push explicitly marked
`[titanium-preview]` publishes a GitHub prerelease **only after both matrix jobs
pass**, containing the tested Windows VST3, universal macOS AU and actual PNGs.
These builds are unsigned previews, not a notarized production release.
