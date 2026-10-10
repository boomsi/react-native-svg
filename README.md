# @boomsi/react-native-svg

> A personal fork of [`react-native-svg`](https://www.npmjs.com/package/react-native-svg) ([software-mansion/react-native-svg](https://github.com/software-mansion/react-native-svg)) with fixes for **text / marker rendering on the Windows Fabric (new architecture) renderer**, and for **marker definitions and mask compositing on Apple platforms** (`<marker>` painted as content at the canvas origin; masked elements losing their content on macOS Fabric). Behavior on Android and on the legacy Windows (Paper) renderer is identical to upstream.

## Base version

Forked from upstream **15.15.5** (main right after `Release 15.15.5`, base commit [`7b2c7d87`](https://github.com/software-mansion/react-native-svg/commit/7b2c7d87)). Update this section whenever syncing with upstream.

## Changes vs. upstream

The Windows Fabric renderer draws the whole document through D2D1's `ID2D1SvgDocument`, which **does not support `text` / `tspan`** (they are silently ignored), so all SVG text disappears. This fork paints text on top via **DirectWrite**:

- `windows/RNSVG/Fabric/`: added real renderables `TextView` / `TSpanView` (previously `UnsupportedSvgView`); `SvgView` collects `TextRecord`s and paints text after `DrawSvgDocument` with `ID2D1DeviceContext::DrawTextLayout` (accurate baseline + textAnchor alignment).
- **Text→TSpan style inheritance**: a `TextContext` carrying textAnchor / fontSize / fontFamily / fontWeight / fill is passed down while recursing (upstream JS wraps raw text into a prop-less `<TSpan>`, so the inheritance semantics were missing).
- **tspan coordinate semantics**: `x/y` on a tspan are treated as absolute values in the text coordinate system and `dy` as a relative increment; `x/y/dx/dy` on `Text` feed the default origin of its children.
- **`dominant-baseline` / `alignment-baseline`**: supports central/middle, text-before-edge/hanging and text-after-edge/bottom vertical positioning; defaults to alphabetic.
- **Nested `<svg>`**: the Fabric path used to skip the whole subtree; nested svgs are now supported and their viewBox transforms accumulate into the text transform.
- **JS-side prop type fixes**: `x/y/dx/dy/rotate` in `TextNativeComponent` / `TSpanNativeComponent` changed from `UnsafeMixed<NumberArray>` to `ReadonlyArray<Float>` to match the native `std::vector<float>`.

### Apple platforms

- **`<marker>` is no longer painted as content**: `RNSVGMarker` overrides `renderTo:rect:` with a no-op. Markers are definitions and must only be drawn through `marker-start/-mid/-end`; d2 and mermaid both emit their `<marker>` elements outside `<defs>`, which left the arrowhead shapes painted into the document flow at the canvas origin.
- **Mask compositing (Apple platforms)**: offscreen bitmaps were sized from `rect` × the backing scale, which under-sizes them for nodes inside a nested `<svg>` — `rect` is in the parent's *user space* there while the CTM additionally applies the outer viewBox scale, so with d2's nested svgs the masked connection lines fell outside the bitmap and vanished (arrowheads survived, as they render outside the mask). macOS also multiplied the backing scale in by hand on top of that. Offscreen bitmaps are now sized per node depth — `rect × getScreenScale()` for top-level nodes (rect in points, unchanged), `rect × scaleOf(CTM)` for nested nodes — with the blend drawn back through a single placement helper (`drawBackImage:…`).

Full design notes, root-cause analysis and the Windows build commands live in [CHANGE.md](./CHANGE.md) (in Chinese).

## Installation

```bash
npm install @boomsi/react-native-svg
```

Usage is identical to upstream `react-native-svg` and it can be used as a 1:1 drop-in replacement. Windows Fabric hosts need to build `RNSVG.dll` from the `windows/` directory in the package (see CHANGE.md for build notes).

---

<p align="center">
  <img src="https://user-images.githubusercontent.com/39658211/200319759-006c214f-941c-496c-a3c2-7de5b7ce33dc.png" width="100%" alt="React Native SVG at Software Mansion" >
</p>

[![Version](https://img.shields.io/npm/v/@boomsi/react-native-svg.svg)](https://www.npmjs.com/package/@boomsi/react-native-svg)
[![NPM](https://img.shields.io/npm/dm/@boomsi/react-native-svg.svg)](https://www.npmjs.com/package/@boomsi/react-native-svg)

`react-native-svg` provides SVG support to React Native on iOS, Android, macOS, Windows, and a compatibility layer for the web.

[Check out the Example app](https://github.com/software-mansion/react-native-svg/tree/main/apps/common/example)

- [Features](#features)
- [Installation](#installation)
- [Troubleshooting](#troubleshooting)
- [Opening issues](#opening-issues)
- [Usage](#usage)
- [Known issues](#known-issues)

## Features

1. Supports most SVG elements and properties (Rect, Circle, Line, Polyline, Polygon, G ...).
2. Easy to [convert SVG code](https://svgr.now.sh/) to react-native-svg.

## Installation

### With expo

> ✅ The [Expo client app](https://expo.io/tools) comes with the native code installed!

Install the JavaScript with:

```bash
npx expo install react-native-svg
```

📚 See the [**Expo docs**](https://docs.expo.io/versions/latest/sdk/svg/) for more info or jump ahead to [Usage](#usage).

### With react-native-cli

1. Install library

   from npm

   ```bash
   npm install react-native-svg
   ```

   from yarn

   ```bash
   yarn add react-native-svg
   ```

2. Link native code

   ```bash
   cd ios && pod install
   ```

## Supported react-native versions

| react-native-svg | react-native |
| ---------------- | ------------ |
| 3.2.0            | 0.29         |
| 4.2.0            | 0.32         |
| 4.3.0            | 0.33         |
| 4.4.0            | 0.38         |
| 4.5.0            | 0.40         |
| 5.1.8            | 0.44         |
| 5.2.0            | 0.45         |
| 5.3.0            | 0.46         |
| 5.4.1            | 0.47         |
| 5.5.1            | >=0.50       |
| >=6              | >=0.50       |
| >=7              | >=0.57.4     |
| >=8              | >=0.57.4     |
| >=9              | >=0.57.4     |
| >=12.3.0         | >=0.64.0     |
| >=15.0.0         | >=0.70.0     |
| >=15.8.0         | >=0.73.0     |
| >=15.13.0        | >=0.78.0     |

## Support for Fabric

[Fabric](https://reactnative.dev/architecture/fabric-renderer) is React Native's new rendering system. As of [version `13.0.0`](https://github.com/react-native-svg/react-native-svg/releases/tag/v13.0.0) of this project, Fabric is supported only for react-native 0.69.0+. Support for earlier versions is not possible due to breaking changes in configuration.

| react-native-svg | react-native |
| ---------------- | ------------ |
| >=13.0.0         | 0.69.0+      |
| >=13.6.0         | 0.70.0+      |
| >=13.10.0        | 0.72.0+      |

## Troubleshooting

### Unexpected behavior

If you have unexpected behavior, please create a clean project with the latest versions of react-native and react-native-svg

```bash
react-native init CleanProject
cd CleanProject/
yarn add react-native-svg
cd ios && pod install && cd ..
```

Make a reproduction of the problem in `App.js`

```bash
react-native run-ios
react-native run-android
```

### Adding Windows support

1. `npx react-native-windows-init --overwrite`
2. `react-native run-windows`

## Opening issues

Verify that it is still an issue with the latest version as specified in the previous step. If so, open a new issue, include the entire `App.js` file, specify what platforms you've tested, and the results of running this command:

```bash
react-native info
```

If you suspect that you've found a spec conformance bug, then you can test using your component in a react-native-web project by forking this codesandbox, to see how different browsers render the same content: <https://codesandbox.io/s/pypn6mn3y7> If any evergreen browser with significant userbase or other svg user agent renders some svg content better, or supports more of the svg and related specs, please open an issue asap.

## Usage

To check how to use the library, see [USAGE.md](https://github.com/react-native-svg/react-native-svg/blob/main/USAGE.md)

## Known issues:

1. Unable to apply focus point of RadialGradient on Android.

## React Native SVG is maintained by Software Mansion

Since 2012 [Software Mansion](https://swmansion.com) is a software agency with experience in building web and mobile apps. We are Core React Native Contributors and experts in dealing with all kinds of React Native issues. We can help you build your next dream product – [Hire us](https://swmansion.com/contact/projects?utm_source=svg&utm_medium=readme).
