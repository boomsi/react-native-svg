#pragma once

#include "SvgView.h"

#include <JSValueComposition.h>
#include <NativeModules.h>
#include "D2DHelpers.h"
#include "SvgFontFields.h"
#include "SvgStrings.h"

namespace winrt::Microsoft::ReactNative {
void WriteValue(IJSValueWriter const &writer, const D2D1_SVG_LENGTH &value) noexcept;
void ReadValue(IJSValueReader const &reader, /*out*/ D2D1_SVG_LENGTH &value) noexcept;
} // namespace winrt::Microsoft::ReactNative

namespace winrt::RNSVG::implementation {

REACT_STRUCT(ColorStruct)
struct ColorStruct {
  REACT_FIELD(type)
  int32_t type{-1};

  REACT_FIELD(payload)
  winrt::Microsoft::ReactNative::Color payload{nullptr};

  REACT_FIELD(brushRef)
  std::wstring brushRef;

  bool operator==(const ColorStruct &rhs) const {
    if (type != rhs.type || brushRef != rhs.brushRef)
      return false;

    // When we move to a RNW version that provides Color::Equals switch to that for the payload comparison
    auto writer = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
    winrt::Microsoft::ReactNative::WriteValue(writer, payload);
    auto rhsWriter = winrt::Microsoft::ReactNative::MakeJSValueTreeWriter();
    winrt::Microsoft::ReactNative::WriteValue(rhsWriter, rhs.payload);
    return winrt::Microsoft::ReactNative::TakeJSValue(writer).Equals(
        winrt::Microsoft::ReactNative::TakeJSValue(rhsWriter));
  }

  bool operator!=(const ColorStruct &rhs) const {
    return !(*this == rhs);
  }
};

HRESULT SetColorMode(
    const SvgView &svgView,
    ID2D1SvgElement &element,
    const std::wstring &attribute,
    const ColorStruct &colorProp) noexcept;

// Currently no good way to do inheritance in REACT_STRUCTS
#define REACT_SVG_RENDERABLE_COMMON_PROPS                      \
  REACT_FIELD(name)                                            \
  std::optional<std::wstring> name;                            \
  REACT_FIELD(opacity)                                         \
  std::optional<float> opacity;                                \
  REACT_FIELD(matrix)                                          \
  std::optional<std::vector<float>> matrix;                    \
  REACT_FIELD(markerStart)                                     \
  std::optional<std::wstring> markerStart;                     \
  REACT_FIELD(markerMid)                                       \
  std::optional<std::wstring> markerMid;                       \
  REACT_FIELD(markerEnd)                                       \
  std::optional<std::wstring> markerEnd;                       \
  REACT_FIELD(clipPath)                                        \
  std::optional<std::wstring> clipPath;                        \
  REACT_FIELD(clipRule)                                        \
  std::optional<D2D1_FILL_MODE> clipRule;                      \
  REACT_FIELD(fill)                                            \
  std::optional<ColorStruct> fill;                             \
  REACT_FIELD(fillOpacity)                                     \
  std::optional<float> fillOpacity;                            \
  REACT_FIELD(fillRule)                                        \
  std::optional<D2D1_FILL_MODE> fillRule;                      \
  REACT_FIELD(stroke)                                          \
  std::optional<ColorStruct> stroke;                           \
  REACT_FIELD(strokeOpacity)                                   \
  std::optional<float> strokeOpacity;                          \
  REACT_FIELD(strokeWidth)                                     \
  std::optional<D2D1_SVG_LENGTH> strokeWidth;                  \
  REACT_FIELD(strokeLinecap)                                   \
  std::optional<uint32_t> strokeLinecap;                       \
  REACT_FIELD(strokeLinejoin)                                  \
  std::optional<D2D1_SVG_LINE_JOIN> strokeLinejoin;            \
  REACT_FIELD(strokeDasharray)                                 \
  std::optional<std::vector<D2D1_SVG_LENGTH>> strokeDasharray; \
  REACT_FIELD(strokeDashoffset)                                \
  std::optional<float> strokeDashoffset;                       \
  REACT_FIELD(strokeMiterlimit)                                \
  std::optional<float> strokeMiterlimit;                       \
  REACT_FIELD(propList)                                        \
  std::optional<std::vector<std::string>> propList;            \
  std::optional<winrt::Microsoft::ReactNative::Color> color;   \
  winrt::Microsoft::ReactNative::ViewProps m_props{nullptr};

#define REACT_SVG_RENDERABLE_COMMON_PROPS_INIT \
  : m_props(props)

#define REACT_BEGIN_SVG_RENDERABLE_COMMON_PROPS_CLONE(TProps) \
     if (cloneFrom) {                                         \
       auto cloneFromProps = cloneFrom.as<TProps>();          \
       name = cloneFromProps->name;                           \
       opacity = cloneFromProps->opacity;                     \
       matrix = cloneFromProps->matrix;                       \
       markerStart = cloneFromProps->markerStart;             \
       markerMid = cloneFromProps->markerMid;                 \
       markerEnd = cloneFromProps->markerEnd;                 \
       clipPath = cloneFromProps->clipPath;                   \
       clipRule = cloneFromProps->clipRule;                   \
       fill = cloneFromProps->fill;                           \
       fillOpacity = cloneFromProps->fillOpacity;             \
       fillRule = cloneFromProps->fillRule;                   \
       stroke = cloneFromProps->stroke;                       \
       strokeOpacity = cloneFromProps->strokeOpacity;         \
       strokeWidth = cloneFromProps->strokeWidth;             \
       strokeLinecap = cloneFromProps->strokeLinecap;         \
       strokeLinejoin = cloneFromProps->strokeLinejoin;       \
       strokeDasharray = cloneFromProps->strokeDasharray;     \
       strokeMiterlimit = cloneFromProps->strokeMiterlimit;   \
       propList = cloneFromProps->propList;                   \
       color = cloneFromProps->color;

#define REACT_END_SVG_RENDERABLE_COMMON_PROPS_CLONE \
     }

// Text 子树的可继承属性上下文：SVG 语义里 font/fill/text-anchor/alignment-baseline
// 沿文本树继承（<text> 上的属性作用于其 <tspan> 子）。JS extractText 会把纯文本包成
// 不带 props 的 <TSpan>，iOS/Android/Paper Windows 在 native 侧走父链继承；Fabric 版
// 由 RecurseRenderNode 递归时显式携带（元素自身值优先，见 ApplyOverrides）。
struct TextContext {
  std::wstring fontFamily;   // 空 = 未设置
  float fontSize{-1.0f};     // <0 = 未设置
  std::wstring fontWeight;   // 空 = 未设置（"bold"/"normal"/数字串）
  std::wstring textAnchor;   // 空 = start（默认）
  std::wstring baselineMode; // 空 = alphabetic（默认）；central/middle/hanging/...
  bool hasFill{false};
  D2D1::ColorF fill{0, 0, 0, 1};
  // <text> 自身的 x/y(+dx/dy)：作为子 TSpan 的默认原点（tspan 未带 x/y 时用它；
  // 带了则是文本坐标系里的绝对值，不能用 translate 叠加——否则 d2 多行标签 x 会翻倍）。
  float originX{0.0f}, originY{0.0f};
  bool originPresent{false};

  // 用 self 的已设置字段覆盖 *this（子元素自身值优先于继承值）。
  void ApplyOverrides(const TextContext &self) noexcept {
    if (!self.fontFamily.empty()) fontFamily = self.fontFamily;
    if (self.fontSize > 0) fontSize = self.fontSize;
    if (!self.fontWeight.empty()) fontWeight = self.fontWeight;
    if (!self.textAnchor.empty()) textAnchor = self.textAnchor;
    if (!self.baselineMode.empty()) baselineMode = self.baselineMode;
    if (self.hasFill) {
      hasFill = true;
      fill = self.fill;
    }
    if (self.originPresent) {
      originPresent = true;
      originX = self.originX;
      originY = self.originY;
    }
  }
};

// 从可选的 fill(ColorStruct)/color props 解析文本填充色；解析不到返回 nullopt。
// type 0 = 具体颜色，type 2 = currentColor（用 color prop 值）。
inline std::optional<D2D1::ColorF> ResolveTextFill(
    const std::optional<ColorStruct> &fill,
    const std::optional<winrt::Microsoft::ReactNative::Color> &color,
    const SvgView &root) noexcept {
  if (fill) {
    const auto &fc = fill.value();
    if (fc.type == 0 && fc.payload)
      return D2DHelpers::AsD2DColor(fc.payload.AsWindowsColor(root.Theme()));
    if (fc.type == 2 && color)
      return D2DHelpers::AsD2DColor(color.value().AsWindowsColor(root.Theme()));
  }
  if (color)
    return D2DHelpers::AsD2DColor(color.value().AsWindowsColor(root.Theme()));
  return std::nullopt;
}

// 从元素自身的 font struct + alignmentBaseline + fill/color props 构造 TextContext
// （Text/TSpan 共用；未设置的字段留空，由 ApplyOverrides 决定是否覆盖继承值）。
inline TextContext BuildTextContext(
    const std::optional<SvgFontFields> &font,
    const std::wstring &alignmentBaseline,
    const std::optional<ColorStruct> &fill,
    const std::optional<winrt::Microsoft::ReactNative::Color> &color,
    const SvgView &root) noexcept {
  TextContext ctx;
  if (font) {
    const auto &f = font.value();
    ctx.fontFamily = f.fontFamily;
    if (f.fontSize.value > 0) ctx.fontSize = f.fontSize.value;
    ctx.fontWeight = f.fontWeight;
    ctx.textAnchor = f.textAnchor;
  }
  ctx.baselineMode = alignmentBaseline;
  if (auto resolved = ResolveTextFill(fill, color, root)) {
    ctx.hasFill = true;
    ctx.fill = resolved.value();
  }
  return ctx;
}

struct __declspec(uuid("a03986c0-b06e-4fb8-a86e-16fcc47b2f31")) RenderableView : public ::IUnknown {
 public:
  RenderableView() = default;

  virtual const wchar_t *GetSvgElementName() noexcept = 0;

  // ComponentView
  void MountChildComponentView(
      const winrt::Microsoft::ReactNative::ComponentView &view,
      const winrt::Microsoft::ReactNative::MountChildComponentViewArgs &args) noexcept;
  void UnmountChildComponentView(
      const winrt::Microsoft::ReactNative::ComponentView &view,
      const winrt::Microsoft::ReactNative::UnmountChildComponentViewArgs &args) noexcept;

  virtual void UpdateProps(
      const winrt::Microsoft::ReactNative::ComponentView & /*view*/,
      const winrt::Microsoft::ReactNative::IComponentProps &props,
      const winrt::Microsoft::ReactNative::IComponentProps &oldProps) noexcept;

  virtual void FinalizeUpates(
      const winrt::Microsoft::ReactNative::ComponentView & /*view*/,
      winrt::Microsoft::ReactNative::ComponentViewUpdateMask mask) noexcept;

  ID2D1SvgElement &Render(const SvgView &svgView, ID2D1SvgDocument &document, ID2D1SvgElement &svgElement) noexcept;

  virtual void OnRender(const SvgView &svgView, ID2D1SvgDocument &document, ID2D1SvgElement & /*svgElement*/) noexcept;
  virtual bool IsSupported() const noexcept;

  // Text 元素（Text/TSpan）：D2D1 SVG 不支持 text/tspan，不 CreateChild 到 D2D1 文档，
  // 而是 RecurseRenderNode 时调 RecordText 收集文字信息到 SvgView，DrawSvgDocument 画完
  // 形状后用 DWrite 自绘（见 SvgView::Draw）。
  virtual bool IsTextElement() const noexcept { return false; }
  // 收集一条文字记录（TSpan 持有 content）；inherited 是父链已合并好的可继承属性
  //（含 <text> 的 x/y 默认原点 originX/originY）。
  virtual void RecordText(
      SvgView &root,
      D2D1_MATRIX_3X2_F accumulatedTransform,
      const TextContext &inherited) noexcept {}
  // 本元素自身的可继承属性（font/fill/alignment-baseline + Text 的原点），
  // RecurseRenderNode 会 ApplyOverrides 到父链 context 上再传给子。
  virtual TextContext GetTextContext(const SvgView &root) const noexcept { return {}; }
  // 元素的 transform matrix（common props.matrix，6 元素）。RecurseRenderNode 累积父链
  // matrix 传给 text 的 RecordText，用于算 text 在 SVG 坐标系的位置。
  virtual std::optional<std::vector<float>> GetMatrix() const noexcept { return std::nullopt; }
  // 元素自身的 marker 引用（Path/Line 等支持 marker 的元素覆盖；默认无）。
  virtual MarkerRefs GetMarkerRefs() const noexcept { return {}; }

  void Invalidate(const winrt::Microsoft::ReactNative::ComponentView &view);

 protected:
  winrt::Microsoft::ReactNative::IComponentProps m_props;

 private:
  winrt::com_ptr<ID2D1SvgElement> m_spD2DSvgElement;
};

template <typename TProps>
void SetCommonSvgProps(
    const SvgView &svgView,
    ID2D1SvgDocument &document,
    ID2D1SvgElement &element,
    const TProps &commonProps) noexcept {
  HRESULT hr = S_OK;
  if (commonProps.color != std::nullopt) {
    auto color = commonProps.color.value().AsWindowsColor(svgView.Theme());
    hr |= element.SetAttributeValue(SvgStrings::colorAttributeName, D2DHelpers::AsD2DColor(color));
  }

  if (commonProps.propList) {
    for (auto &prop : commonProps.propList.value()) {
      if (prop == "fill") {
        if (commonProps.fill != std::nullopt)
          hr |= SetColorMode(svgView, element, SvgStrings::fillAttributeName, commonProps.fill.value());
        else
          hr |= element.SetAttributeValue(
              SvgStrings::fillAttributeName,
              D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG,
              SvgStrings::noneAttributeValue);
      } else if (prop == "fillOpacity") {
        if (commonProps.fillOpacity != std::nullopt)
          hr |= element.SetAttributeValue(SvgStrings::fillOpacityAttributeName, commonProps.fillOpacity.value());
      } else if (prop == "fillRule") {
        if (commonProps.fillRule != std::nullopt) {
          hr |= element.SetAttributeValue(SvgStrings::fillRuleAttributeName, commonProps.fillRule.value());
        }
      } else if (prop == "stroke") {
        if (commonProps.stroke != std::nullopt)
          hr |= SetColorMode(svgView, element, SvgStrings::strokeAttributeName, commonProps.stroke.value());
        else
          hr |= element.SetAttributeValue(
              SvgStrings::strokeAttributeName,
              D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG,
              SvgStrings::noneAttributeValue);
      } else if (prop == "strokeWidth") {
        if (commonProps.strokeWidth != std::nullopt)
          hr |= element.SetAttributeValue(SvgStrings::strokeWidthAttributeName, commonProps.strokeWidth.value());
      } else if (prop == "strokeOpacity") {
        if (commonProps.strokeOpacity != std::nullopt)
          hr |= element.SetAttributeValue(SvgStrings::strokeOpacityAttributeName, commonProps.strokeOpacity.value());
      } else if (prop == "strokeDasharray") {
        if (commonProps.strokeDasharray != std::nullopt && !commonProps.strokeDasharray->empty()) {
          winrt::com_ptr<ID2D1SvgStrokeDashArray> dashArray;
          document.CreateStrokeDashArray(
              &commonProps.strokeDasharray.value()[0],
              static_cast<UINT32>(commonProps.strokeDasharray.value().size()),
              dashArray.put());
          hr |= element.SetAttributeValue(SvgStrings::strokeDashArrayAttributeName, dashArray.get());
        }
      } else if (prop == "strokeDashoffset") {
        if (commonProps.strokeDashoffset != std::nullopt) {
          hr |= element.SetAttributeValue(
              SvgStrings::strokeDashOffsetAttributeName, commonProps.strokeDashoffset.value());
        }
      } else if (prop == "strokeLinecap") {
        if (commonProps.strokeLinecap != std::nullopt) {
          static D2D1_SVG_LINE_CAP supportedCaps[] = {
              D2D1_SVG_LINE_CAP_BUTT, D2D1_SVG_LINE_CAP_ROUND, D2D1_SVG_LINE_CAP_SQUARE};
          hr |= element.SetAttributeValue(
              SvgStrings::strokeLinecapAttributeName, supportedCaps[commonProps.strokeLinecap.value()]);
        }
      } else if (prop == "strokeLinejoin") {
        if (commonProps.strokeLinejoin != std::nullopt) {
          static D2D1_SVG_LINE_JOIN supportedJoins[] = {
              D2D1_SVG_LINE_JOIN_MITER, D2D1_SVG_LINE_JOIN_ROUND, D2D1_SVG_LINE_JOIN_BEVEL};
          hr |= element.SetAttributeValue(
              SvgStrings::strokeLinejoinAttributeName, supportedJoins[commonProps.strokeLinejoin.value()]);
        }
      } else if (prop == "strokeMiterlimit") {
        if (commonProps.strokeMiterlimit != std::nullopt) {
          hr |= element.SetAttributeValue(
              SvgStrings::strokeMiterLimitAttributeName, commonProps.strokeMiterlimit.value());
        }
      }
    }
  }

  if (commonProps.clipPath != std::nullopt) {
    std::wstring namedRefStr = L"url(#" + commonProps.clipPath.value() + L")";
    hr |= element.SetAttributeValue(
        SvgStrings::clipPathAttributeName,
        D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG,
        namedRefStr.c_str());
  }

  if (commonProps.clipRule != std::nullopt) {
    hr |= element.SetAttributeValue(SvgStrings::clipRuleAttributeName, commonProps.clipRule.value());
  }

  if (commonProps.name != std::nullopt)
    hr |= element.SetAttributeValue(
        SvgStrings::idAttributeName,
        D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG,
        commonProps.name.value().c_str());

  if (commonProps.opacity != std::nullopt)
    hr |= element.SetAttributeValue(SvgStrings::opacityAttributeName, commonProps.opacity.value());

  if (commonProps.matrix != std::nullopt) {
    auto &matrix = commonProps.matrix.value();
    hr |= element.SetAttributeValue(
        SvgStrings::transformAttributeName,
        D2D1_MATRIX_3X2_F{matrix[0], matrix[1], matrix[2], matrix[3], matrix[4], matrix[5]});
  }

  assert(hr == S_OK);
}
} // namespace winrt::RNSVG::implementation

template <typename TProps, typename TUserData>
void RegisterRenderableComponent(
    const winrt::hstring &name,
    const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept {
  builder.AddViewComponent(name, [](winrt::Microsoft::ReactNative::IReactViewComponentBuilder const &builder) noexcept {
    builder.SetComponentViewInitializer([](const winrt::Microsoft::ReactNative::ComponentView &view) noexcept {
      auto userData = winrt::make_self<TUserData>();
      view.UserData(*userData);
    });
    builder.SetCreateProps(
        [](winrt::Microsoft::ReactNative::ViewProps props, const winrt::Microsoft::ReactNative::IComponentProps &cloneFrom) noexcept { return winrt::make<TProps>(props, cloneFrom); });
    builder.SetUpdatePropsHandler([](const winrt::Microsoft::ReactNative::ComponentView &view,
                                     const winrt::Microsoft::ReactNative::IComponentProps &newProps,
                                     const winrt::Microsoft::ReactNative::IComponentProps &oldProps) noexcept {
      auto userData = winrt::get_self<TUserData>(view.UserData());
      userData->UpdateProps(view, newProps, oldProps);
    });
    builder.SetFinalizeUpdateHandler([](const winrt::Microsoft::ReactNative::ComponentView &view,
                                        const winrt::Microsoft::ReactNative::ComponentViewUpdateMask mask) noexcept {
      auto userData = winrt::get_self<TUserData>(view.UserData());
      userData->FinalizeUpates(view, mask);
    });
    builder.SetMountChildComponentViewHandler(
        [](const winrt::Microsoft::ReactNative::ComponentView &view,
           const winrt::Microsoft::ReactNative::MountChildComponentViewArgs &args) noexcept {
          auto userData = winrt::get_self<TUserData>(view.UserData());
          return userData->MountChildComponentView(view, args);
        });
    builder.SetUnmountChildComponentViewHandler(
        [](const winrt::Microsoft::ReactNative::ComponentView &view,
           const winrt::Microsoft::ReactNative::UnmountChildComponentViewArgs &args) noexcept {
          auto userData = winrt::get_self<TUserData>(view.UserData());
          return userData->UnmountChildComponentView(view, args);
        });
  });
}
