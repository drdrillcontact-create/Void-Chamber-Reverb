# Production reference pass

The user-supplied `void-chamber-da-production.png` is the layout target.
The backdrop is an AI-assisted reconstruction of the chamber without baked-in
labels or controls; all parameter controls, meters and text remain rendered by
JUCE. It is not claimed to be a pixel-identical extraction of the reference.

- Hero scene above a quiet lower control deck, wider machined header tray.
- Larger dark rotary bodies, thin accent rings and narrower indicators.
- Smaller, fine-stroked decay arc below the hero; lighter typography/tracking.
- Continuous rectangular meters and fine advanced-panel symbols.
- Corner resizing constrained to 3:2. Host-forced off-ratio sizes fill the client
  using the same two-axis transform for background, controls and hit testing.
- Snapshot test renders 1536×1024, 960×640, 1200×900 and 1800×900 and checks
  that the bezel reaches both sides. CI uploads all four images.

Review the actual editor PNGs, not just the background asset, for visual approval.
