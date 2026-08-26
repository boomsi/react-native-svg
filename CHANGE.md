# CHANGE

## Windows Fabric：SVG 文本不渲染（DWrite 自绘方案）

### 问题原因

D2D1 的 SVG 引擎（`ID2D1SvgDocument`）**不支持 text / tspan / marker 元素**（Microsoft 文档
svg-support 的支持列表里没有它们，遇到即忽略）。Windows Fabric 路径用
`DrawSvgDocument` 整文档渲染，因此所有图表文字（标签、刻度）全部消失。
Windows Paper 路径不受影响——它逐元素自绘，text 走 DWrite（`windows/RNSVG/TSpanView.cpp`）。

早期尝试"补 renderable 让 D2D1 渲染 text 元素"的方向不可行——引擎本身不支持，
怎么补都不会画。

### 修改方案

Fabric 路径改为 **DWrite 叠加自绘**：`DrawSvgDocument` 画完形状后，
用 `ID2D1DeviceContext::DrawTextLayout` 把文字叠加画上去。

- 新增 `Fabric/TSpanView`、`Fabric/TextView`（从 UnsupportedSvgView 改为真实 renderable）：
  `RecurseRenderNode` 遍历到 Text/TSpan 时不再 CreateChild 进 D2D1 文档，
  而是 `RecordText` 收集文字信息（content / font / fill / x / y + 累积 transform）到 SvgView。
- `SvgView`：新增 `TextRecord` 列表 + `ComputeViewBoxTransform`（复现 preserveAspectRatio
  的 viewBox→surface 映射）+ `DrawTextRecords`（TextLayout 精确 baseline +
  textAnchor 水平对齐）。
- `RecurseRenderNode` 增加 accumulatedTransform 参数：累积父链 `matrix`；
  Text 自身的 x/y 作为 translate 累积到子 TSpan（修 dot 等把 x/y 放在
  `<text>` 元素上导致的文本叠加）。
- 嵌套 `<svg>` 支持：d2 输出嵌套 svg，而 SvgView 不是 RenderableView，
  `try_as` 失败会整棵子树跳过（d2 整片空白的根因）。嵌套分支 `CreateChild("svg")`
  并设其 viewBox/width/height，同时把嵌套 viewBox 变换累积进文字的 transform。
- Props 类型修正：JS 侧 `x/y/dx/dy/rotate` 传的是 number 数组
  （`extractLengthList` 的产物），native 用 `std::vector<float>` 接；
  TS spec 里 `x/y` 从 `UnsafeMixed<NumberArray>` 改为 `ReadonlyArray<Float>`
  （与 matrix 同型，native 才能收到）。font 用嵌套 struct `SvgFontFields`
  （fontFamily / fontSize / fontWeight / textAnchor）。
- `RNSVG.vcxproj`：`dwrite.lib` + `/utf-8`（中文注释在 GBK 代码页下会触发
  C4819 连锁语法错误）+ `ResolveAssemblyWarnOrErrorOnTargetArchitectureMismatch=None`
  （RNW 预编译 winmd 是 x86，运行时由宿主提供 x64）。

### 构建注意

- 必须经 solution 构建（`SolutionPath` 检查）且带 `-restore`（NuGet 生成
  midlrt.rsp，否则 MIDL 编译 IDL 报 namespace 语法错误）。
- 产物 `RNSVG.dll` 需按宿主要求改名为 `RNSVGImpl.dll`（避免与 winmd 同名触发
  MSBuild 程序集验证问题，见宿主 Fuse.csproj 注释）。

### 当前状态（未完成）

- mermaid / dot / echarts / vega / plantuml 的文字已能显示（位置对齐有少量偏差可再调）。
- **d2 文字仍不显示**：诊断到 Text/TSpan 的 `x/y` props 未 populate 到 native
  （font 的嵌套 struct 已能收到，x/y 数组不行）——TS spec 的类型修改待验证，
  正在排查 RNW 对该字段类型的映射。
- 仓库内含**临时诊断代码**（`DebugTrace` 调试面板绘制在 SVG 左侧、
  `Text.tsx` 的 `[rnsvg-text]` 日志），问题解决后移除。
