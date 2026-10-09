#include "pch.h"
#include "MarkerView.h"

namespace winrt::RNSVG::implementation {

void RegisterMarkerComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept {
  RegisterRenderableComponent<MarkerProps, MarkerView>(L"RNSVGMarker", builder);
}

} // namespace winrt::RNSVG::implementation