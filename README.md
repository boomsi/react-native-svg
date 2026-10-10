# @boomsi/react-native-svg

A personal fork of [`react-native-svg`](https://github.com/software-mansion/react-native-svg), based on **v15.15.5**.

## Changes vs. upstream

### Windows (Fabric renderer)

1. **Text** — D2D1's `ID2D1SvgDocument` ignores `text` / `tspan`, so all diagram text was missing; text is now drawn on top with DirectWrite (font, size, weight, textAnchor, `dominant-baseline`).
2. **Nested `<svg>`** — the inner svg's viewBox/width/height were never applied (d2 emits nested svgs), clipping diagram edges; they are now set and their transform accumulates into the text transform.
3. **Arrowheads** — D2D1 ignores `<marker>` as well, so arrowheads were missing; markers are now collected and drawn manually following SVG marker semantics (`refX/refY`, `markerUnits`, `orient`, viewBox mapping).

### Apple platforms (iOS / macOS / tvOS / visionOS)

4. **`<marker>` is no longer painted as content** — markers are definitions, and d2/mermaid emit theirs outside `<defs>`, which used to paint stray arrowheads at the canvas origin.
5. **`mask` / `clipPath` that are direct `<svg>` children** — still registered, but no longer painted over the diagram (d2's mask used to cover the connection lines).
6. **Mask / filter offscreen bitmaps** — now sized and placed correctly; masked content (e.g. d2's connection lines) used to be drawn outside the bitmap and vanished. The placement is derived from the content's bounding box, which also handles UIKit's flipped CTM (iOS).
7. **Build** — added a `std::vector<Float>` overload so the shared text props code compiles with the codegen output of this fork's specs.

## Usage

Drop-in replacement for `react-native-svg`:

```bash
npm install @boomsi/react-native-svg
```

Windows Fabric hosts additionally need to build `RNSVG.dll` from the package's `windows/` directory.

See [CHANGE.md](./CHANGE.md) for detailed root-cause write-ups and build notes (in Chinese).
