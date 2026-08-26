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
    REACT_END_SVG_RENDERABLE_COMMON_PROPS_CLONE
  }

  void SetProp(uint32_t hash, winrt::hstring propName, winrt::Microsoft::ReactNative::IJSValueReader value) noexcept {
    winrt::Microsoft::ReactNative::ReadProp(hash, propName, value, *this);
  }

  REACT_SVG_RENDERABLE_COMMON_PROPS;

  // 文字内容：extractText(prop, false) 在 TSpan 上产出 content=String(children)。
  REACT_FIELD(content)
  std::wstring content;
  // x/y/dx/dy/rotate：JS extractLengthList 序列化为空格分隔数字字符串，取首值。
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

// TSpan 持有实际文字。D2D1 SVG 不支持 tspan 元素，所以不 CreateChild（IsTextElement=true
// 让 RecurseRenderNode 走 RecordText 分支），而是把 content/font/fill/x/y + 累积 transform
// 收集到 SvgView，DrawSvgDocument 画完形状后用 DWrite 自绘。
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

  void RecordText(SvgView &root, D2D1_MATRIX_3X2_F accumulatedTransform) noexcept override {
    auto props = m_props.as<TSpanProps>();
    if (props->content.empty()) return;

    // extractLengthList 空格分隔数字字符串，取首值；parse 失败回 0。
    auto parseFirst = [](const std::wstring &s) -> float {
      if (s.empty()) return 0.0f;
      try {
        return std::stof(s);
      } catch (...) {
        return 0.0f;
      }
    };
    float tx = parseFirst(props->x) + parseFirst(props->dx);
    float ty = parseFirst(props->y) + parseFirst(props->dy);
    // 累积 transform（父 g matrix + 自身 matrix）应用到 (x,y) → SVG 坐标系文字基线点。
    // D2D1::Matrix3x2F 继承 D2D1_MATRIX_3X2_F（布局相同），reinterpret 即可。
    const D2D1::Matrix3x2F &m = reinterpret_cast<const D2D1::Matrix3x2F &>(accumulatedTransform);
    D2D1_POINT_2F pos = m.TransformPoint({tx, ty});

    TextRecord rec;
    rec.content = props->content;
    rec.x = pos.x;
    rec.y = pos.y;
    if (props->font) {
      auto &f = props->font.value();
      rec.fontFamily = f.fontFamily;
      rec.fontSize = (f.fontSize.value != 0) ? f.fontSize.value : 16.0f;
      rec.fontWeight = (f.fontWeight == L"bold") ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
      rec.textAnchor = f.textAnchor;
    }
    // fill: ColorStruct type 0=native color, 2=currentColor→用 color prop, 其他(brush ref 等)→黑。
    if (props->fill) {
      auto &fc = props->fill.value();
      if (fc.type == 0 && fc.payload) {
        rec.fill = D2DHelpers::AsD2DColor(fc.payload.AsWindowsColor(root.Theme()));
      } else if (fc.type == 2 && props->color) {
        rec.fill = D2DHelpers::AsD2DColor(props->color.value().AsWindowsColor(root.Theme()));
      }
    } else if (props->color) {
      rec.fill = D2DHelpers::AsD2DColor(props->color.value().AsWindowsColor(root.Theme()));
    }
    root.AddTextRecord(std::move(rec));
  }
};

void RegisterTSpanComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept {
  RegisterRenderableComponent<TSpanProps, TSpanView>(L"RNSVGTSpan", builder);
}

} // namespace winrt::RNSVG::implementation
