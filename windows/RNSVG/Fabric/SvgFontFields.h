#pragma once

#include <NativeModules.h>
#include <d2d1svg.h>

namespace winrt::RNSVG::implementation {

// font 嵌套 struct（JS extractFont 产出的 font 对象）。D2D1_SVG_LENGTH 的
// ReadValue/WriteValue 特化在 RenderableView.h/.cpp 声明实现，TSpanProps/TextProps
// include 本头 + RenderableView.h 即可读取 fontSize。
REACT_STRUCT(SvgFontFields)
struct SvgFontFields {
  REACT_FIELD(fontFamily)
  std::wstring fontFamily;
  REACT_FIELD(fontSize)
  D2D1_SVG_LENGTH fontSize;
  REACT_FIELD(fontWeight)
  std::wstring fontWeight;
  REACT_FIELD(fontStyle)
  std::wstring fontStyle;
  REACT_FIELD(textAnchor)
  std::wstring textAnchor;
};

} // namespace winrt::RNSVG::implementation
