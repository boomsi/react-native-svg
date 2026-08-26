#include "pch.h"
#include "TextView.h"
#include "SvgFontFields.h"
#include "SvgView.h"
#include "RenderableView.h"
#include <d2d1.h>

namespace winrt::RNSVG::implementation {

REACT_STRUCT(TextProps)
struct TextProps : winrt::implements<TextProps, winrt::Microsoft::ReactNative::IComponentProps> {
  TextProps(const winrt::Microsoft::ReactNative::ViewProps &props, const winrt::Microsoft::ReactNative::IComponentProps &cloneFrom) REACT_SVG_RENDERABLE_COMMON_PROPS_INIT
  {
    REACT_BEGIN_SVG_RENDERABLE_COMMON_PROPS_CLONE(TextProps)
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

// Text 是 container：extractText 把纯文本 children 包成 <TSpan> 子节点（content=null），
// 字体/锚点/填充等可继承属性都在 Text 自身。D2D1 SVG 不支持 text 元素，IsTextElement=true
// 让 RecurseRenderNode 不 CreateChild，而是把自身属性经 GetTextContext 合入继承链，
// 再递归子 TSpan（其 RecordText 收集文字）。
struct TextView : winrt::implements<TextView, winrt::Windows::Foundation::IInspectable, RenderableView> {
 public:
  TextView() = default;

  const wchar_t *GetSvgElementName() noexcept override {
    return L"text";
  }

  bool IsTextElement() const noexcept override {
    return true;
  }

  std::optional<std::vector<float>> GetMatrix() const noexcept override {
    auto props = m_props.as<TextProps>();
    return props->matrix;
  }

  // Text 自身不持文字（在 TSpan），RecordText 空；可继承属性经 GetTextContext 下传。
  void RecordText(
      SvgView & /*root*/,
      D2D1_MATRIX_3X2_F /*accumulatedTransform*/,
      const TextContext & /*inherited*/) noexcept override {}

  // font/fill/baseline + 自身 x/y(+dx/dy) 作为子 TSpan 的默认原点（originX/originY）。
  // tspan 自带 x/y 时是文本坐标系里的绝对值，不做 translate 叠加（避免 d2 多行标签
  // x 翻倍），见 TSpanView::RecordText。
  TextContext GetTextContext(const SvgView &root) const noexcept override {
    auto props = m_props.as<TextProps>();
    TextContext ctx = BuildTextContext(props->font, props->alignmentBaseline, props->fill, props->color, root);
    auto firstOr0 = [](const std::vector<float> &v) -> float {
      return v.empty() ? 0.0f : v[0];
    };
    ctx.originX = firstOr0(props->x) + firstOr0(props->dx);
    ctx.originY = firstOr0(props->y) + firstOr0(props->dy);
    ctx.originPresent = true;
    return ctx;
  }
};

void RegisterTextComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept {
  RegisterRenderableComponent<TextProps, TextView>(L"RNSVGText", builder);
}

} // namespace winrt::RNSVG::implementation
