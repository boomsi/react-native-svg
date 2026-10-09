#pragma once

#include "RenderableView.h"

namespace winrt::RNSVG::implementation {

// RNSVGMarker：只登记 marker 定义（props + 子组件树），不往主 D2D1 文档建元素。
// SvgView::DrawMarkerRecords 按引用把内容渲染进独立 marker 文档并手动放置
// （D2D1 的 ID2D1SvgDocument 不支持 <marker>，整文档渲染会静默忽略）。
// IMarkerView 接口本体在 SvgView.h（SvgView 收集时按它 try_as）。

REACT_STRUCT(MarkerProps)
struct MarkerProps : winrt::implements<MarkerProps, winrt::Microsoft::ReactNative::IComponentProps> {
  MarkerProps(
      const winrt::Microsoft::ReactNative::ViewProps &props,
      const winrt::Microsoft::ReactNative::IComponentProps &cloneFrom)
      : m_props(props) {
    if (cloneFrom) {
      auto cloneFromProps = cloneFrom.as<MarkerProps>();
      name = cloneFromProps->name;
      refX = cloneFromProps->refX;
      refY = cloneFromProps->refY;
      markerWidth = cloneFromProps->markerWidth;
      markerHeight = cloneFromProps->markerHeight;
      markerUnits = cloneFromProps->markerUnits;
      orient = cloneFromProps->orient;
      minX = cloneFromProps->minX;
      minY = cloneFromProps->minY;
      vbWidth = cloneFromProps->vbWidth;
      vbHeight = cloneFromProps->vbHeight;
    }
  }

  void SetProp(uint32_t hash, winrt::hstring propName, winrt::Microsoft::ReactNative::IJSValueReader value) noexcept {
    winrt::Microsoft::ReactNative::ReadProp(hash, propName, value, *this);
  }

  REACT_FIELD(name)
  std::optional<std::wstring> name;  // = Marker 的 id
  // refX/refY/markerWidth/markerHeight：JS 侧可能是数字（JSX）或字符串（SvgXml 属性），
  // 复用 D2D1_SVG_LENGTH 的 ReadValue 特化（RenderableView.cpp）同时兼容两种。
  REACT_FIELD(refX)
  std::optional<D2D1_SVG_LENGTH> refX;
  REACT_FIELD(refY)
  std::optional<D2D1_SVG_LENGTH> refY;
  REACT_FIELD(markerWidth)
  std::optional<D2D1_SVG_LENGTH> markerWidth;
  REACT_FIELD(markerHeight)
  std::optional<D2D1_SVG_LENGTH> markerHeight;
  REACT_FIELD(markerUnits)
  std::optional<std::wstring> markerUnits;  // "strokeWidth"(默认) / "userSpaceOnUse"
  REACT_FIELD(orient)
  std::optional<std::wstring> orient;  // "auto" / "auto-start-reverse" / 数字串（度）
  REACT_FIELD(minX)
  std::optional<float> minX;
  REACT_FIELD(minY)
  std::optional<float> minY;
  REACT_FIELD(vbWidth)
  std::optional<float> vbWidth;
  REACT_FIELD(vbHeight)
  std::optional<float> vbHeight;

 private:
  winrt::Microsoft::ReactNative::ViewProps m_props{nullptr};
};

struct MarkerView
    : winrt::implements<MarkerView, winrt::Windows::Foundation::IInspectable, RenderableView, IMarkerView> {
 public:
  MarkerView() = default;

  // RenderableView：marker 不往主文档建元素，RecurseRenderNode 在调用 Render 前
  // 就用 IMarkerView 拦截；这两个 override 只是满足基类（不会被调用）。
  const wchar_t *GetSvgElementName() noexcept override {
    return L"marker";
  }

  void OnRender(const SvgView & /*svgView*/, ID2D1SvgDocument & /*document*/, ID2D1SvgElement & /*element*/) noexcept override {}

  MarkerDefAttrs Attrs() override {
    MarkerDefAttrs attrs;
    auto props = m_props.as<MarkerProps>();
    if (props->name) attrs.name = props->name.value();
    auto lengthValue = [](const std::optional<D2D1_SVG_LENGTH> &v, float fallback) {
      return v ? v->value : fallback;
    };
    attrs.refX = lengthValue(props->refX, 0.0f);
    attrs.refY = lengthValue(props->refY, 0.0f);
    attrs.markerWidth = lengthValue(props->markerWidth, 3.0f);
    attrs.markerHeight = lengthValue(props->markerHeight, 3.0f);
    attrs.strokeWidthUnits = !props->markerUnits || props->markerUnits.value() != L"userSpaceOnUse";
    attrs.orient = props->orient.value_or(L"0");
    attrs.vbMinX = props->minX.value_or(0.0f);
    attrs.vbMinY = props->minY.value_or(0.0f);
    attrs.vbWidth = props->vbWidth.value_or(0.0f);
    attrs.vbHeight = props->vbHeight.value_or(0.0f);
    attrs.hasViewBox = attrs.vbWidth > 0 && attrs.vbHeight > 0;
    return attrs;
  }
};

void RegisterMarkerComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept;

} // namespace winrt::RNSVG::implementation