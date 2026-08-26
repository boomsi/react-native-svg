#include "pch.h"
#include "TSpanView.h"
#include "SvgFontFields.h"
#include "SvgView.h"
#include "D2DHelpers.h"

#include <dwrite.h>

namespace winrt::RNSVG::implementation {

REACT_STRUCT(TSpanProps)
struct TSpanProps : winrt::implements<TSpanProps, winrt::Microsoft::ReactNative::IComponentProps> {
  TSpanProps(const winrt::Microsoft::ReactNative::ViewProps &props, const winrt::Microsoft::ReactNative::IComponentProps &cloneFrom) REACT_SVG_RENDERABLE_COMMON_PROPS_INIT
  {
    REACT_BEGIN_SVG_RENDERABLE_COMMON_PROPS_CLONE(TSpanProps)
      content = cloneFromProps->content;
      x = cloneFromProps->x;
      y = cloneFromProps->y;
      dx = cloneFromProps->dx;
      dy = cloneFromProps->dy;
      rotate = cloneFromProps->rotate;
      font = cloneFromProps->font;
      alignmentBaseline = cloneFromProps->alignmentBaseline;
    REACT_END_SVG_RENDERABLE_COMMON_PROPS_CLONE
  }

  void SetProp(uint32_t hash, winrt::hstring propName, winrt::Microsoft::ReactNative::IJSValueReader value) noexcept {
    winrt::Microsoft::ReactNative::ReadProp(hash, propName, value, *this);
  }

  REACT_SVG_RENDERABLE_COMMON_PROPS;

  // 文字内容：extractText(prop, false) 在 TSpan 上产出 content=String(children)。
  REACT_FIELD(content)
  std::wstring content;
  // x/y/dx/dy/rotate：JS extractLengthList 传 number 数组 [26.5]，用 vector<float> 收。
  REACT_FIELD(x)
  std::vector<float> x;
  REACT_FIELD(y)
  std::vector<float> y;
  REACT_FIELD(dx)
  std::vector<float> dx;
  REACT_FIELD(dy)
  std::vector<float> dy;
  REACT_FIELD(rotate)
  std::vector<float> rotate;
  REACT_FIELD(font)
  std::optional<SvgFontFields> font;
  // alignment-baseline（JS extractText 已把 dominant-baseline 折叠进来）。
  REACT_FIELD(alignmentBaseline)
  std::wstring alignmentBaseline;
};

// TSpan 持有实际文字。D2D1 SVG 不支持 tspan 元素，所以不 CreateChild（IsTextElement=true
// 让 RecurseRenderNode 走 RecordText 分支），而是把 content + 自身 x/y/dx/dy + 累积
// transform + 继承来的 font/fill/anchor 收集到 SvgView，DrawSvgDocument 画完形状后自绘。
// 注意 JS extractText 会把 <text>label</text> 包成无 props 的 TSpan，font/textAnchor/
// fill 都得从父 Text 的 TextContext 继承（inherited 参数，RecurseRenderNode 已合并）。
struct TSpanView : winrt::implements<TSpanView, winrt::Windows::Foundation::IInspectable, RenderableView> {
 public:
  TSpanView() = default;

  const wchar_t *GetSvgElementName() noexcept override {
    return L"tspan";
  }

  bool IsTextElement() const noexcept override {
    return true;
  }

  std::optional<std::vector<float>> GetMatrix() const noexcept override {
    auto props = m_props.as<TSpanProps>();
    return props->matrix;
  }

  TextContext GetTextContext(const SvgView &root) const noexcept override {
    auto props = m_props.as<TSpanProps>();
    return BuildTextContext(props->font, props->alignmentBaseline, props->fill, props->color, root);
  }

  void RecordText(
      SvgView &root,
      D2D1_MATRIX_3X2_F accumulatedTransform,
      const TextContext &inherited) noexcept override {
    auto props = m_props.as<TSpanProps>();
    if (props->content.empty()) return;

    // x/y/dx/dy 是 number 数组，取首值。tspan 自带 x/y 时是文本坐标系里的绝对值；
    // 未带时用 <text> 的原点（originX/originY，dot/echarts 等单行标签全靠它）。
    // dy 按相对增量处理（d2 多行每行 dy=行高增量，从 text 基线累加）。
    auto firstOr0 = [](const std::vector<float> &v) -> float {
      return v.empty() ? 0.0f : v[0];
    };
    float tx = !props->x.empty() ? props->x[0] + firstOr0(props->dx)
                                 : inherited.originX + firstOr0(props->dx);
    float ty = !props->y.empty() ? props->y[0] + firstOr0(props->dy)
                                 : inherited.originY + firstOr0(props->dy);
    // 累积 transform（父链 matrix + 自身 matrix）应用到 (x,y) → SVG 坐标系文字基线点。
    // D2D1::Matrix3x2F 继承 D2D1_MATRIX_3X2_F（布局相同），reinterpret 即可。
    const D2D1::Matrix3x2F &m = reinterpret_cast<const D2D1::Matrix3x2F &>(accumulatedTransform);
    D2D1_POINT_2F pos = m.TransformPoint({tx, ty});

    TextRecord rec;
    rec.content = props->content;
    rec.x = pos.x;
    rec.y = pos.y;
    rec.fontFamily = inherited.fontFamily;
    rec.fontSize = inherited.fontSize > 0 ? inherited.fontSize : 16.0f;
    rec.fontWeight = inherited.fontWeight;
    rec.textAnchor = inherited.textAnchor;
    rec.baselineMode = inherited.baselineMode;
    if (inherited.hasFill) rec.fill = inherited.fill;
    root.AddTextRecord(std::move(rec));
  }
};

void RegisterTSpanComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept {
  RegisterRenderableComponent<TSpanProps, TSpanView>(L"RNSVGTSpan", builder);
}

} // namespace winrt::RNSVG::implementation
