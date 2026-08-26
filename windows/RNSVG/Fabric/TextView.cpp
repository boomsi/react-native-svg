#include "pch.h"
#include "TextView.h"
#include "SvgFontFields.h"
#include "RenderableView.h"

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
    REACT_END_SVG_RENDERABLE_COMMON_PROPS_CLONE
  }

  void SetProp(uint32_t hash, winrt::hstring propName, winrt::Microsoft::ReactNative::IJSValueReader value) noexcept {
    winrt::Microsoft::ReactNative::ReadProp(hash, propName, value, *this);
  }

  REACT_SVG_RENDERABLE_COMMON_PROPS;

  REACT_FIELD(x)
  std::wstring x;
  REACT_FIELD(y)
  std::wstring y;
  REACT_FIELD(dx)
  std::wstring dx;
  REACT_FIELD(dy)
  std::wstring dy;
  REACT_FIELD(rotate)
  std::wstring rotate;
  REACT_FIELD(font)
  std::optional<SvgFontFields> font;
};

// Text 是 container：extractText 把纯文本 children 包成 <TSpan> 子节点（content=null）。
// D2D1 SVG 不支持 text 元素，IsTextElement=true 让 RecurseRenderNode 不 CreateChild，
// 而是递归子 TSpan（其 RecordText 收集文字），并累积 Text 自身的 matrix 到子。
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

  // Text 的 x/y 作为 translate 累积到子（TSpan 的 pos 计算会含此偏移）。
  D2D1_POINT_2F GetTextTranslate() const noexcept override {
    auto props = m_props.as<TextProps>();
    auto parseFirst = [](const std::wstring &s) -> float {
      if (s.empty()) return 0.0f;
      try {
        return std::stof(s);
      } catch (...) {
        return 0.0f;
      }
    };
    return {parseFirst(props->x), parseFirst(props->y)};
  }

  // Text 自身不持文字（在 TSpan），RecordText 空。RecurseRenderNode 仍会递归其子 TSpan。
};

void RegisterTextComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept {
  RegisterRenderableComponent<TextProps, TextView>(L"RNSVGText", builder);
}

} // namespace winrt::RNSVG::implementation
