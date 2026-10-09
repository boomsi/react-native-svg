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

### 当前状态（第二轮修复，2026-08-20 Windows 实机验证通过）

dot / echarts / vega / mermaid / plantuml / d2 六引擎文本渲染正常
（构建命令见下）。

上一轮遗留的根因复盘：78fb392 的 `vector<float>` 修复**从未编译成功**——
`TextView.cpp` 把 `props->x`（vector<float>）赋给 `g_trace.textX`（wstring），
类型不匹配编不过，所以部署的 dll 仍是 wstring 旧版（x/y 永远收不到，
JS 发的是 number 数组）。且 `TSpanNativeComponent.ts` 的 x/y 类型没同步改。
本轮修复（代码已完成，需重建部署）：

- **Text→TSpan 继承**（此前缺失的最大根因）：JS `extractText` 会把 `<text>label</text>`
  的纯文本包成**无 props 的 `<TSpan>`**，textAnchor/fontSize/fontFamily/fill 全在
  `<text>` 上；iOS/Android/Paper Windows 在 native 侧走父链继承，Fabric 版此前只读
  TSpan 自身 props → 所有文字都按 start 锚 + 16px + Arial + 黑色画（echarts/vega
  竖刻度"靠右"、全引擎字号/颜色不对的原因）。现新增 `TextContext`
  （RenderableView.h），RecurseRenderNode 递归时显式携带，元素自身值经
  `ApplyOverrides` 优先。
- **x/y 管道**：修掉 `g_trace` 编译错误（连同调试面板一起删除）；`TSpanNativeComponent.ts`
  的 x/y/dx/dy/rotate 同步改 `ReadonlyArray<Float>`（与 Text/matrix 同型，native
  `std::vector<float>` 才收得到）。
- **tspan 坐标语义**：Text 的 x/y 不再作 translate 叠加，改为经 TextContext 的
  `originX/originY` 作子 TSpan 默认原点——tspan 自带 x 时是文本坐标系**绝对值**
  （叠加会把 d2 多行标签的 x 翻倍）；dy 按相对增量处理（d2 每行 dy=行高）。
  Text 的 dx/dy 也并入原点。
- **dominant/alignment-baseline**：`extractText` 把 SVG `dominant-baseline` 折叠成
  `alignmentBaseline` 传出（显式 alignment-baseline 优先）；C++ props 声明该字段；
  `DrawTextRecords` 实现 central/middle（行盒中心对齐 y，zrender 的 echarts/vega
  刻度标签全靠它）、text-before-edge/hanging、text-after-edge/bottom 的竖直定位，
  默认 alphabetic（y=字母基线）。
- **fontWeight** 从 font struct 一路带到 DWrite（"bold"/数字≥600 → BOLD）。
- 调试代码全部移除：`DebugTrace` 面板（绿色字）、`[rnsvg-text]` 日志、
  supramark `DiagramNode.tsx` 的 `[diag-d2]` 日志。

已知未做（按需再补）：d2 代码块 `y="1.0em"` 的 em 单位（extractLengthList 产出
字符串数组，vector<float> 收不到 → 回退原点）；plantuml 的 `textLength` 压缩
（忽略，宽度按自然排版）；`rotate`（未用）。

### 构建注意（同前）

- **可用的构建命令**（2026-08-20 验证）：WSL interop 下直接
  `'/mnt/c/Program Files/Microsoft Visual Studio/18/Community/MSBuild/Current/Bin/MSBuild.exe' windows/CanvasSubsystem.sln -t:RNSVG -p:Configuration=Release -p:Platform=x64 -p:UseFabric=true -nologo -v:m`
  （在 /mnt/d/Repo/loom 下执行。**必须带 `-p:UseFabric=true`**——不带会走
  vcxproj 的 Windows Store(UWP) 分支，VS18 没有 v143 UWP 工具集报 MSB8020；
  2022 BuildTools 那套缺 .NET SDK（sln restore 碰 C# 项目即 MSB4276），别用。
  NuGet 产物已还原过，无需 -restore。）
- 部署命名：运行时实际按 winmd 名解析 `RNSVG.dll`（Fuse bin 里曾同时存在
  679KB 的 RNSVG.dll=真实现 与 140KB 的 RNSVGImpl.dll=坏产物——后者是 8/19 一次
  `-p:TargetName=RNSVGImpl` 实验的失败残留，绿面板证明加载的是前者）。
  稳妥做法：构建产出后**同名两份都放**（RNSVG.dll + cp 成 RNSVGImpl.dll），覆盖
  `Fuse_Win/bin/x64/{Debug,Release}/` 与其下 `net10.0-*/win-x64(/AppX)` 的旧文件。
- WSL interop（binfmt WSLInterop）在本机不稳定，会被反复冲掉；失效时从 Windows 侧
  重注册：`wsl -u root -- sh -c "echo ':WSLInterop:M::MZ::/init:PF' > /proc/sys/fs/binfmt_misc/register"`。

## Windows Fabric：嵌套 <svg> 的 viewBox 从未生效（d2 图右缘被裁 + 文字变换累积死代码）

### 问题原因

d2 输出双层 svg：内层 `<svg viewBox="11 -1 112 255">` 带**非零 min-x/min-y**，
内容坐标整体偏移 (11, -1)。`RecurseRenderNode` 的嵌套分支此前这样读子 SvgView 的 props：

    auto nestedProps = child.UserData().try_as<IComponentProps>();
    auto svgProps = nestedProps ? nestedProps.try_as<SvgViewProps>() : nullptr;

`child.UserData()` 放的是 **SvgView 本身**（`implements<SvgView, IInspectable, ISvgView>`，
props 存在它的私有 `m_props` 成员里），不实现 `IComponentProps` → 该 try_as **恒为
null**。后果：

- 内层 svg 的 viewBox/width/height 从未设到 D2D1 元素上 → 内层内容按原始坐标绘制，
  被内层视口在右/上边缘裁掉 min-x/min-y 个单位 → **d2 图最右描边丢失、顶部描边削半**
  （线上实测：内容偏移与 viewBox 偏移量完全吻合）；
- 下面那段"嵌套 viewBox 变换累积进文字 transform"（nestedTextTransform）同样被
  `if (svgProps)` 挡掉，成为死代码 → DWrite 文字也落在原始坐标。

### 修改方案

- `ISvgView` 增加 `Props()`（返回本 SvgView 的 `SvgViewProps`，`SvgView` 实现之）；
  嵌套分支改经 `nestedSvg->Props()` 读取（SvgView.h/.cpp）。
- 其余逻辑不变：设嵌套 svg 的 viewBox + width/height（=vbWidth/vbHeight，d2 如此）
  后，D2D1 按 SVG 语义正确应用 min-x/min-y 平移并精确贴边；文字累积公式
  `(nW − nvbW·ns)/2 − minX·ns` 与 D2D1 的映射一致，两者对齐。

### 验证

- 独立 D2D1 程序（同几何 439×1000，根 viewBox `0 0 112 255` + 内层 svg）四个变体：
  - 内层 svg 不设属性（=修复前）：右描边整体缺失、底部边框画到视口右缘截断、
    顶部描边只画一半 —— 与线上截图逐像素吻合；
  - 内层 svg 设 viewBox+宽高（=本修复）：平移正确、右描边出现、内容精确贴满视口；
  - 内层 svg 宽高 + `<g transform="translate(-11,1)">`、根 viewBox 直接带偏移：
    同样正确（D2D1 根/嵌套 viewBox 的 min-x/min-y 语义均符合 SVG 规范）。
- 待实机复测 d2 消息右缘。

### RNR 时代的构建命令（替换上文 loom/CanvasSubsystem 版）

    MSBuild.exe D:\Repo\Fuse_Win\windows\SparkNative.sln -t:RNSVG -restore \
      -p:Configuration=Release -p:Platform=x64 -p:UseFabric=true -p:TargetName=RNSVGImpl -nologo -v:m

产物落 `spark/windows/x64/Release/`（RNSVG.winmd + RNSVGImpl.dll）。源码单点:RNR 的
`node_modules/react-native-svg` 是指向本仓的 junction（RNR package.json 以
`link:../../react-native-svg` 声明，`scripts/link-local-packages.js` postinstall 维护，
metro 配置已把本仓加入 watchFolders/blockList 并按 real 路径解析），改这里即构建源，
不需再同步 node_modules。
注意：`UseExperimentalNuget=false` 会让 RNSVG 触发对 RNW 源码工程的
ProjectReference（Microsoft.ReactNative → fmt/Folly），首次/清理后构建需
`-restore`（boost 走 NuGet）且耗时较长。

## Windows Fabric：marker（箭头）手动绘制（D2D1 不支持 <marker>）

### 问题原因

d2 / mermaid / plantuml / graphviz 等用 `<marker>` 定义 + `marker-start/mid/end="url(#id)"`
引用画箭头。D2D1 的 `ID2D1SvgDocument` 不支持 `<marker>` 元素（整文档渲染静默忽略），
上游原版在 Windows 上也从未实现（Paper 的 marker 处理是注释掉的 TODO；Fabric 把
`RNSVGMarker` 注册成 UnsupportedSvgComponent）。结果：Windows 上所有图表的箭头全部
丢失（iOS / Android / macOS 上游有完整实现，不受影响）。

### 修改方案

沿用"文字 DWrite 叠加"的思路，marker 也改为收集后手动绘制：

- `RNSVGMarker` 从 UnsupportedSvgView 改为真实 `MarkerView`（Fabric/MarkerView.h/.cpp）：
  只登记定义（refX/refY/markerWidth/Height/viewBox/markerUnits/orient + 子树），
  不往主文档建元素。数值 props 兼容数字（JSX）与字符串（SvgXml 属性），复用
  `D2D1_SVG_LENGTH` 的 ReadValue 特化。
- 公共 props 宏增加 `markerStart/markerMid/markerEnd`（含 clone），
  `RenderableView::GetMarkerRefs()` 虚函数由 Path/Line 覆盖。JS 侧无需改动
  （`PathNativeComponent` 已带这三个 prop；`polygon`/`polyline` 在 JS 层就是 Path）。
- `RecurseRenderNode` 收集两样东西：① marker 定义（IMarkerView 拦截，含"不放进 defs"
  的根级兜底）；② 带引用的元素锚点——path 把 D2D1 元素上的 `d` 取回
  `ID2D1SvgPathData` → `ID2D1PathGeometry1` 按弧长采样首/末端点 + 切线；line 读
  x1..y2；都记元素局部坐标 + 累积变换 + stroke-width。
- `DrawMarkerRecords`（在 DrawSvgDocument + 文字叠加之后）：marker 子树渲染进独立
  D2D1 文档（root 尺寸 = marker 视口、viewBox = 定义值），按 SVG marker 语义放置：

      translate(端点) · rotate(orient) · scale(markerUnits) · translate(-refV)

  orient 支持 auto / auto-start-reverse / 固定角度（可带 deg）；refX/refY 按默认
  xMidYMid meet 经 viewBox→视口映射；最终经元素累积变换 × 根 viewBox 映射绘制。

### 已知限制

- `marker-mid` 未实现（d2/mermaid/plantuml/graphviz 的箭头都在路径两端）。
- marker 子内容里的 text/tspan 不绘制（与主文档同一限制，marker 内极罕见）。
- Paper（旧架构）未动，仍不显示 marker。

### 验证

- RNSVG.vcxproj 编译通过（RNSVGImpl.dll 743,936B）。
- 实机：待 Fuse 复测 d2 图箭头（database 侧应出现箭头，方向沿弧线切线）。
