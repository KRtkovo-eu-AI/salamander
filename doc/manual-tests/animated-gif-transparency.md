# Manual Test: 8-bit Animated GIF Transparency

This regression check ensures that semi-transparent pixels in paletted GIF animations
render without colour banding or grid artefacts.

## Required asset
- A dithered 8-bit animated GIF with transparency (for example, a ScreenToGif capture
  exported with the gifski encoder).

## Steps
1. Open the GIF in PictView.
2. Let the animation loop at least once.
3. Inspect regions that fade in/out or show cursor trails.

## Expected result
- The animation uses the authored colours without blue/green/purple speckles.
- No grid artefacts or haloing appears around semi-transparent content.
- Pixels with fractional alpha look smooth across frames while fully transparent
  regions remain transparent.
