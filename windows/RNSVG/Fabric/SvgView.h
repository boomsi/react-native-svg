#pragma once

#include <unknwn.h>

#include <d2d1_3.h>
#include <dwrite.h>
#include <unordered_map>
#include <NativeModules.h>
#pragma push_macro("X86")
#undef X86
#include <winrt/Microsoft.ReactNative.Composition.Experimental.h>
#include <winrt/Microsoft.ReactNative.h>
#include <JSValueComposition.h>
#pragma pop_macro("X86")

namespace winrt::RNSVG::implementation {

D2D1_SVG_ASPECT_ALIGN AlignToAspectAlign(const std::string &align) noexcept;

enum class MeetOrSlice {
    Meet = 0,
    Slice = 1,
};

REACT_STRUCT(SvgViewProps)
struct SvgViewProps : winrt::implements<SvgViewProps, winrt::Microsoft::ReactNative::IComponentProps> {
  SvgViewProps(const winrt::Microsoft::ReactNative::ViewProps &props, const winrt::Microsoft::ReactNative::IComponentProps& cloneFrom);

  void SetProp(uint32_t hash, winrt::hstring propName, winrt::Microsoft::ReactNative::IJSValueReader value) noexcept;

  REACT_FIELD(minX)
  std::optional<float> minX;
  REACT_FIELD(minY)
  std::optional<float> minY;
  REACT_FIELD(vbWidth)
  std::optional<float> vbWidth;
  REACT_FIELD(vbHeight)
  std::optional<float> vbHeight;
  REACT_FIELD(align)
  std::optional<std::string> align;
  REACT_FIELD(meetOrSlice)
  std::optional<MeetOrSlice> meetOrSlice;
  REACT_FIELD(color)
  winrt::Microsoft::ReactNative::Color color{nullptr};
 private:
  winrt::Microsoft::ReactNative::ViewProps m_props{nullptr};
};

struct __declspec(uuid("ed381ffa-461a-48Bf-a3c0-5d9a42eecd30")) ISvgView : public ::IUnknown {
  virtual void Invalidate() = 0;
  // 本 SvgView 当前的 props（minX/minY/vbWidth/vbHeight 等）。RecurseRenderNode
  // 给嵌套 <svg> 建 D2D1 内层 svg 元素时要读子 SvgView 的 viewBox props —— 只能
  // 从这里拿：child.UserData() 是 SvgView 本身（只实现 IInspectable/ISvgView），
  // props 存在它的 m_props 成员里，try_as<IComponentProps>() 恒为 null。
  virtual winrt::com_ptr<SvgViewProps> Props() = 0;
};

// DWrite 自绘文字记录：D2D1 SVG 不支持 text/tspan 元素，DrawSvgDocument 会忽略它们。
// RecurseRenderNode 遍历到 TSpan 时调 SvgView::AddTextRecord 收集，Draw 在画完形状后
// 用 DWrite 叠加（累积 transform + viewBox→surface 映射算 surface 坐标）。
// font/anchor/baseline 已含 Text→TSpan 继承（TextContext），fill 默认黑。
struct TextRecord {
  std::wstring content;
  std::wstring fontFamily;
  float fontSize{16.0f};
  std::wstring fontWeight;   // 空/"normal"/"bold"/数字串，DrawTextRecords 折算 DWrite weight
  D2D1::ColorF fill{0, 0, 0, 1};
  float x{0.0f}, y{0.0f};  // SVG 坐标（已含累积 transform）
  std::wstring textAnchor;   // start(默认)/middle/end，DWrite 水平对齐用
  std::wstring baselineMode; // 空=alphabetic(默认)；central/middle/hanging/text-before-edge...
};

// 元素自身的 marker 引用（marker-start/mid/end，已由 JS extractProps 从
// url(#id) 剥成纯 id）。Fabric 下 D2D1 不渲染 <marker>，RecurseRenderNode 收集
// 引用元素锚点后由 SvgView 手动绘制箭头（见 SvgView::DrawMarkerRecords）。
struct MarkerRefs {
  std::optional<std::wstring> start;
  std::optional<std::wstring> mid;
  std::optional<std::wstring> end;
  bool empty() const noexcept {
    return !start && !mid && !end;
  }
};

// 已归一化的 marker 定义属性（RNSVGMarker 组件，MarkerView 从 props 折算）。
struct MarkerDefAttrs {
  std::wstring name;                        // = Marker 的 id
  float refX{0.0f}, refY{0.0f};
  float markerWidth{3.0f}, markerHeight{3.0f};  // JS 默认 3×3（strokeWidth units）
  float vbMinX{0.0f}, vbMinY{0.0f}, vbWidth{0.0f}, vbHeight{0.0f};
  bool hasViewBox{false};
  bool strokeWidthUnits{true};              // markerUnits == strokeWidth（SVG 默认）
  std::wstring orient;                      // "auto" / "auto-start-reverse" / 数字串（度）
};

// RNSVGMarker 的实现接口：SvgView 需要它的 props + 子组件树来把内容渲染进独立
// marker 文档（D2D1 不支持 <marker>，DrawMarkerRecords 手动放置与绘制）。
struct __declspec(uuid("6f2f8a5e-3c1d-4b9a-9e2c-8d4b7a1f0c33")) IMarkerView : public ::IUnknown {
  virtual MarkerDefAttrs Attrs() = 0;
};

// marker 引用元素（带 marker-start/mid/end 的 path/line）：几何锚点已在元素局部
// 坐标系算好（端点 + 切线角），绘制时经 accumulatedTransform 映射到根用户坐标。
struct MarkerRefRecord {
  MarkerRefs refs;
  D2D1_MATRIX_3X2_F accumulatedTransform{1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};
  D2D1_POINT_2F startPoint{0.0f, 0.0f}, endPoint{0.0f, 0.0f};
  float startAngle{0.0f}, endAngle{0.0f};  // 度（局部坐标系的切线方向）
  float strokeWidth{1.0f};                 // markerUnits=strokeWidth 的缩放基准
  bool hasStart{false}, hasEnd{false};
};

struct MarkerDef {
  winrt::com_ptr<IMarkerView> view{nullptr};
  winrt::Microsoft::ReactNative::ComponentView componentView{nullptr};
};

struct SvgView : winrt::implements<SvgView, winrt::Windows::Foundation::IInspectable, ISvgView> {
 public:

  SvgView(const winrt::Microsoft::ReactNative::Composition::Experimental::ICompositionContext &compContext);

  // Overrides
  // IInternalCreateVisual
  winrt::Microsoft::ReactNative::Composition::Experimental::IVisual CreateInternalVisual();

  // ComponentView
  void UpdateProps(
      const winrt::Microsoft::ReactNative::ComponentView & /*view*/,
      const winrt::Microsoft::ReactNative::IComponentProps &newProps,
      const winrt::Microsoft::ReactNative::IComponentProps & /*oldProps*/) noexcept;
  void UpdateLayoutMetrics(
      const winrt::Microsoft::ReactNative::LayoutMetrics &metrics,
      const winrt::Microsoft::ReactNative::LayoutMetrics &oldMetrics);
  void MountChildComponentView(
      const winrt::Microsoft::ReactNative::ComponentView& view,
      const winrt::Microsoft::ReactNative::MountChildComponentViewArgs& args) noexcept;
  void UnmountChildComponentView(
      const winrt::Microsoft::ReactNative::ComponentView& view,
      const winrt::Microsoft::ReactNative::UnmountChildComponentViewArgs& args) noexcept;

  void FinalizeUpates(
      const winrt::Microsoft::ReactNative::ComponentView & /*view*/,
      winrt::Microsoft::ReactNative::ComponentViewUpdateMask mask) noexcept;

  void OnThemeChanged() noexcept;
  void OnMounted() noexcept;
  void OnUnmounted() noexcept;

  void Initialize(const winrt::Microsoft::ReactNative::ComponentView & /*view*/) noexcept;

  static void RegisterComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept;

  void Invalidate();
  winrt::com_ptr<SvgViewProps> Props() override { return m_props; }
  winrt::Microsoft::ReactNative::Composition::Theme Theme() const noexcept;

  // RecurseRenderNode 遍历到 TSpan 时调用，收集文字记录供 Draw 用 DWrite 自绘。
  void AddTextRecord(TextRecord &&record) noexcept { m_textRecords.push_back(std::move(record)); }

  // RecurseRenderNode 遍历到 RNSVGMarker / 带 marker 引用的元素时调用（绘图阶段收集）。
  void AddMarkerDef(const winrt::com_ptr<IMarkerView> &view, const winrt::Microsoft::ReactNative::ComponentView &componentView) noexcept;
  void AddMarkerRef(MarkerRefRecord &&record) noexcept { m_markerRefs.push_back(std::move(record)); }

 private:
  void Draw(
      const winrt::Microsoft::ReactNative::Composition::ViewComponentView &view,
      ID2D1DeviceContext &context,
      winrt::Windows::Foundation::Size const &size) noexcept;

  // viewBox→surface 映射（preserveAspectRatio），DWrite text 坐标用。
  D2D1::Matrix3x2F ComputeViewBoxTransform(winrt::Windows::Foundation::Size size, float &outScale) noexcept;
  // DWrite 自绘 m_textRecords（DrawSvgDocument 画完形状后叠加）。
  void DrawTextRecords(
      const winrt::com_ptr<ID2D1DeviceContext> &deviceContext,
      winrt::Windows::Foundation::Size size) noexcept;
  // 手动绘制 marker（箭头）：D2D1 不支持 <marker>，把 marker 内容画进独立
  // SvgView 文档后按 SVG marker 语义（refX/refY + orient + markerUnits + viewBox
  // 映射）放置到引用元素的端点上（DrawSvgDocument + DrawTextRecords 之后叠加）。
  void DrawMarkerRecords(
      const winrt::com_ptr<ID2D1DeviceContext> &deviceContext,
      winrt::Windows::Foundation::Size size) noexcept;

  bool m_isMounted{false};
  winrt::Microsoft::ReactNative::Composition::Experimental::ISpriteVisual m_visual{nullptr};
  winrt::Microsoft::ReactNative::LayoutMetrics m_layoutMetrics{{0, 0, 0, 0}, 1.0};
  winrt::Microsoft::ReactNative::Composition::Experimental::ICompositionContext m_compContext{nullptr};
  winrt::weak_ref<winrt::Microsoft::ReactNative::Composition::ViewComponentView> m_wkView;
  D2D1_SVG_ASPECT_ALIGN m_aspectAlign;
  winrt::com_ptr<SvgViewProps> m_props;
  std::vector<TextRecord> m_textRecords;
  std::vector<MarkerRefRecord> m_markerRefs;
  std::unordered_map<std::wstring, MarkerDef> m_markerDefs;  // key = Marker 的 id

  // Shared
  Microsoft::ReactNative::IReactContext m_reactContext{nullptr};
};
} // namespace winrt::RNSVG::implementation
