# @boomsi/react-native-svg

> [`react-native-svg`](https://www.npmjs.com/package/react-native-svg)（[software-mansion/react-native-svg](https://github.com/software-mansion/react-native-svg)）的个人 fork，仅对 **Windows Fabric（新架构）** 的文本渲染做了修复，iOS / Android / macOS 及 Windows 旧架构（Paper）行为与上游一致。

## 基线版本

基于上游 **15.15.5**（`Release 15.15.5` 之后的 main，基线 commit [`7b2c7d87`](https://github.com/software-mansion/react-native-svg/commit/7b2c7d87)）。后续同步上游时请更新本节。

## 相对上游的改动

Windows Fabric 路径使用 D2D1 的 `ID2D1SvgDocument` 整文档渲染，而该引擎**不支持 `text` / `tspan`**（遇到即忽略），导致所有图表文字消失。本 fork 改为 **DWrite 叠加自绘**：

- `windows/RNSVG/Fabric/`：新增 `TextView` / `TSpanView` 真实 renderable（原为 UnsupportedSvgView）；`SvgView` 收集 `TextRecord`，在 `DrawSvgDocument` 之后用 `ID2D1DeviceContext::DrawTextLayout` 叠加绘制文字（精确 baseline + textAnchor 对齐）。
- **Text→TSpan 样式继承**：新增 `TextContext` 递归携带 textAnchor / fontSize / fontFamily / fontWeight / fill（上游 JS 会把纯文本包成无 props 的 `<TSpan>`，继承语义此前缺失）。
- **tspan 坐标语义**：tspan 自带 `x/y` 按文本坐标系绝对值处理，`dy` 按相对增量；Text 的 `x/y/dx/dy` 并入子元素默认原点。
- **`dominant-baseline` / `alignment-baseline`**：支持 central/middle、text-before-edge/hanging、text-after-edge/bottom 竖直定位，默认 alphabetic。
- **嵌套 `<svg>`**：Fabric 路径原先整棵子树跳过，现支持嵌套 svg 并把其 viewBox 变换累积进文字变换。
- **JS 侧 props 类型修正**：`TextNativeComponent` / `TSpanNativeComponent` 的 `x/y/dx/dy/rotate` 从 `UnsafeMixed<NumberArray>` 改为 `ReadonlyArray<Float>`，与 native `std::vector<float>` 对齐。

完整方案、根因复盘与 Windows 构建命令见 [CHANGE.md](./CHANGE.md)。

## 安装

```bash
npm install @boomsi/react-native-svg
```

用法与上游 `react-native-svg` 相同，可 1:1 替换。Windows Fabric 宿主需自行从包内 `windows/` 目录构建 `RNSVG.dll`（构建注意事项见 CHANGE.md）。

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
