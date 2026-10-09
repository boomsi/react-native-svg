#include "pch.h"

#include "SvgView.h"

#include "D2DHelpers.h"
#include "GroupView.h"
#include <dwrite.h>
#include <d2d1.h>

#include <AutoDraw.h>
#include <winrt/Microsoft.ReactNative.Composition.Experimental.h>
#include <CompositionSwitcher.Experimental.interop.h>
#include <winrt/Windows.Foundation.Collections.h>

#include <d3d11_4.h>

namespace winrt::RNSVG::implementation {

SvgViewProps::SvgViewProps(const winrt::Microsoft::ReactNative::ViewProps& props, const winrt::Microsoft::ReactNative::IComponentProps& cloneFrom)
  : m_props(props)
{
  if (cloneFrom) {
    auto cloneFromProps = cloneFrom.as<SvgViewProps>();
    minX = cloneFromProps->minX;
    minY = cloneFromProps->minY;
    vbWidth = cloneFromProps->vbWidth;
    vbHeight = cloneFromProps->vbHeight;
    align = cloneFromProps->align;
    meetOrSlice = cloneFromProps->meetOrSlice;
    color = cloneFromProps->color;
  }
}

void SvgViewProps::SetProp(
    uint32_t hash,
    winrt::hstring propName,
    winrt::Microsoft::ReactNative::IJSValueReader value) noexcept {
  winrt::Microsoft::ReactNative::ReadProp(hash, propName, value, *this);
}

SvgView::SvgView(const winrt::Microsoft::ReactNative::Composition::Experimental::ICompositionContext &compContext)
    : m_compContext(compContext) {}

winrt::Microsoft::ReactNative::Composition::Experimental::IVisual SvgView::CreateInternalVisual() {
  m_visual = m_compContext.CreateSpriteVisual();
  m_visual.Comment(L"SVGRoot");
  return m_visual;
}

void SvgView::MountChildComponentView(
    const winrt::Microsoft::ReactNative::ComponentView &,
    const winrt::Microsoft::ReactNative::MountChildComponentViewArgs &) noexcept {
  Invalidate();
}

void SvgView::UnmountChildComponentView(
    const winrt::Microsoft::ReactNative::ComponentView &,
    const winrt::Microsoft::ReactNative::UnmountChildComponentViewArgs &) noexcept {
  Invalidate();
}

void SvgView::OnThemeChanged() noexcept {
  Invalidate();
}

void SvgView::OnMounted() noexcept {
  m_isMounted = true;
  Invalidate();
}

void SvgView::OnUnmounted() noexcept {
  m_isMounted = false;
}

D2D1_SVG_ASPECT_ALIGN AlignToAspectAlign(const std::string &align) noexcept {
  if (align.compare("xMinYMin") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MIN_Y_MIN;
  else if (align.compare("xMidYMin") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MIN;
  else if (align.compare("xMaxYMin") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MIN;
  else if (align.compare("xMinYMid") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MIN_Y_MID;
  else if (align.compare("xMidYMid") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MID;
  else if (align.compare("xMaxYMid") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MID;
  else if (align.compare("xMinYMax") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MIN_Y_MAX;
  else if (align.compare("xMidYMax") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MAX;
  else if (align.compare("xMaxYMax") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MAX;
  else if (align.compare("none") == 0)
    return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_NONE;

  assert(false);
  return D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_NONE;
}

void SvgView::UpdateProps(
    const winrt::Microsoft::ReactNative::ComponentView & /*view*/,
    const winrt::Microsoft::ReactNative::IComponentProps &newProps,
    const winrt::Microsoft::ReactNative::IComponentProps & /*oldProps*/) noexcept {
  m_props = newProps.as<SvgViewProps>();
  
  if (m_props->align) {
    m_aspectAlign = AlignToAspectAlign(m_props->align.value());
  } else {
    m_aspectAlign = D2D1_SVG_ASPECT_ALIGN::D2D1_SVG_ASPECT_ALIGN_NONE;
  }
}

void SvgView::FinalizeUpates(
    const winrt::Microsoft::ReactNative::ComponentView & /*view*/,
    winrt::Microsoft::ReactNative::ComponentViewUpdateMask) noexcept {
  Invalidate(); // Move to finalize
}

void SvgView::Initialize(const winrt::Microsoft::ReactNative::ComponentView &sender) noexcept {
  auto view = sender.as<winrt::Microsoft::ReactNative::Composition::ViewComponentView>();
  m_wkView = view;

  sender.as<winrt::Microsoft::ReactNative::Composition::Experimental::IInternalCreateVisual>()
      .CreateInternalVisualHandler([wkThis = get_weak()](const winrt::Microsoft::ReactNative::ComponentView &) {
        return wkThis.get()->CreateInternalVisual();
      });

  sender.LayoutMetricsChanged(
      [wkThis = get_weak()](
          const winrt::Windows::Foundation::IInspectable &, const winrt::Microsoft::ReactNative::LayoutMetricsChangedArgs &args) {
        if (auto strongThis = wkThis.get()) {
          strongThis->UpdateLayoutMetrics(args.NewLayoutMetrics(), args.OldLayoutMetrics());
        }
      });

  view.ThemeChanged(
      [wkThis = get_weak()](const winrt::Windows::Foundation::IInspectable & /*sender*/, const winrt::Windows::Foundation::IInspectable & /*args*/) {
        if (auto strongThis = wkThis.get()) {
          strongThis->OnThemeChanged();
        }
      });

  view.Mounted([wkThis = get_weak()](
                   const winrt::Windows::Foundation::IInspectable & /*sender*/, const winrt::Microsoft::ReactNative::ComponentView &) {
    if (auto strongThis = wkThis.get()) {
      strongThis->OnMounted();
    }
  });

  view.Unmounted([wkThis = get_weak()](
                     const winrt::Windows::Foundation::IInspectable & /*sender*/, const winrt::Microsoft::ReactNative::ComponentView &) {
    if (auto strongThis = wkThis.get()) {
      strongThis->OnUnmounted();
    }
  });
}

void SvgView::RegisterComponent(const winrt::Microsoft::ReactNative::IReactPackageBuilderFabric &builder) noexcept {
  builder.AddViewComponent(
      L"RNSVGSvgView", [](winrt::Microsoft::ReactNative::IReactViewComponentBuilder const &builder) noexcept {
        builder.SetCreateProps([](winrt::Microsoft::ReactNative::ViewProps props,
                                  const winrt::Microsoft::ReactNative::IComponentProps &cloneFrom) noexcept {
          return winrt::make<SvgViewProps>(props, cloneFrom);
        });
        auto compBuilder =
            builder.as<winrt::Microsoft::ReactNative::Composition::IReactCompositionViewComponentBuilder>();

        compBuilder.SetViewComponentViewInitializer(
            [](const winrt::Microsoft::ReactNative::ComponentView &view) noexcept {
              auto userData = winrt::make_self<SvgView>(
                  view.as<winrt::Microsoft::ReactNative::Composition::Experimental::IInternalComponentView>()
                      .CompositionContext());
              userData->Initialize(view);
              view.UserData(*userData);
            });

        compBuilder.SetViewFeatures(
            winrt::Microsoft::ReactNative::Composition::ComponentViewFeatures::Default &
            ~winrt::Microsoft::ReactNative::Composition::ComponentViewFeatures::Background);

        builder.SetUpdatePropsHandler([](const winrt::Microsoft::ReactNative::ComponentView &view,
                                         const winrt::Microsoft::ReactNative::IComponentProps &newProps,
                                         const winrt::Microsoft::ReactNative::IComponentProps &oldProps) noexcept {
          auto userData = winrt::get_self<SvgView>(view.UserData());
          userData->UpdateProps(view, newProps, oldProps);
        });

        builder.SetFinalizeUpdateHandler(
            [](const winrt::Microsoft::ReactNative::ComponentView &view,
               const winrt::Microsoft::ReactNative::ComponentViewUpdateMask mask) noexcept {
              auto userData = winrt::get_self<SvgView>(view.UserData());
              userData->FinalizeUpates(view, mask);
            });

        builder.SetMountChildComponentViewHandler(
            [](const winrt::Microsoft::ReactNative::ComponentView &view,
               const winrt::Microsoft::ReactNative::MountChildComponentViewArgs &args) noexcept {
              auto userData = winrt::get_self<SvgView>(view.UserData());
              return userData->MountChildComponentView(view, args);
            });

        builder.SetUnmountChildComponentViewHandler(
            [](const winrt::Microsoft::ReactNative::ComponentView &view,
               const winrt::Microsoft::ReactNative::UnmountChildComponentViewArgs &args) noexcept {
              auto userData = winrt::get_self<SvgView>(view.UserData());
              return userData->UnmountChildComponentView(view, args);
            });
      });
}

void SvgView::UpdateLayoutMetrics(
    const winrt::Microsoft::ReactNative::LayoutMetrics &metrics,
    const winrt::Microsoft::ReactNative::LayoutMetrics &oldMetrics) {
  m_layoutMetrics = metrics;

  if (metrics != oldMetrics) {
    Invalidate();
  }
}

void RecurseRenderNode(
    SvgView *root,
    const winrt::Microsoft::ReactNative::ComponentView &view,
    ID2D1SvgDocument &document,
    ID2D1SvgElement &svgElement,
    D2D1_MATRIX_3X2_F accumulatedTransform,
    const TextContext &inheritedTextCtx) noexcept {
  for (auto const &child : view.Children()) {
    auto renderable = child.UserData().try_as<RenderableView>();

    if (renderable && renderable->IsSupported()) {
      // 累积子元素的 transform matrix（common props.matrix，6 元素）到父链。
      D2D1_MATRIX_3X2_F childTransform = accumulatedTransform;
      auto childMatrix = renderable->GetMatrix();
      if (childMatrix && childMatrix->size() >= 6) {
        auto &m = childMatrix.value();
        childTransform = D2D1::Matrix3x2F(m[0], m[1], m[2], m[3], m[4], m[5]) * accumulatedTransform;
      }

      if (renderable->IsTextElement()) {
        // text/tspan：D2D1 SVG 不支持，不 CreateChild。RecordText 收集文字信息
        //（content + 几何 x/y/dx/dy + 累积 transform + 继承的 font/fill/anchor）到
        // SvgView，DrawSvgDocument 画完形状后用 DWrite 自绘。仍递归子（Text 的子是
        // TSpan）。Text 的 x/y 不做 translate 叠加，作为默认原点经 context 下传——
        // tspan 自带 x 时是文本坐标系绝对值，叠加会把 d2 多行标签的 x 翻倍。
        // 可继承属性（font/textAnchor/fill/baseline/原点）：元素自身值优先合并进父链
        // context 再传下去——JS 会把纯文本包成无 props 的 TSpan，不继承则 anchor/
        // fontSize/fill 全丢（iOS/Android/Paper 在 native 侧走父链，Fabric 在此显式传）。
        TextContext childCtx = inheritedTextCtx;
        childCtx.ApplyOverrides(renderable->GetTextContext(*root));
        renderable->RecordText(*root, childTransform, childCtx);
        RecurseRenderNode(root, child, document, svgElement, childTransform, childCtx);
      } else {
        ID2D1SvgElement &newElement = renderable->Render(*root, document, svgElement);
        RecurseRenderNode(root, child, document, newElement, childTransform, inheritedTextCtx);
      }
    } else {
      // 嵌套 <svg>（d2 等）：SvgView 不是 RenderableView，上面 try_as 失败会跳过，
      // 导致内层 svg 整棵子树不渲染（d2 整片空白的根因）。这里 CreateChild svg +
      // 递归其子，让内层 svg 的 shape/text 进当前文档。
      auto nestedSvg = child.UserData().try_as<ISvgView>();
      if (nestedSvg) {
        winrt::com_ptr<ID2D1SvgElement> nestedElem;
        svgElement.CreateChild(L"svg", nestedElem.put());
        if (nestedElem) {
          // 设嵌套 svg 的 viewBox + width/height（用嵌套 SvgView 的 props）。
          // d2 内层 svg 带 viewBox（如 "11 -1 112 255"，带非零 min-x/min-y），
          // 不设的话 D2D1 不建立内层坐标系，内容按原始坐标绘制、被视口裁掉
          // min-x 个单位（d2 图右缘描边丢失的根因）。
          // props 必须经 ISvgView::Props() 拿：child.UserData() 是 SvgView 本身
          // （只实现 IInspectable/ISvgView），此前 try_as<IComponentProps>() 恒为
          // null，这段设置从未生效（含下面的文字变换累积）。
          auto svgProps = nestedSvg->Props();
          if (svgProps) {
            if (svgProps->vbWidth || svgProps->vbHeight) {
              std::wstring vb = std::to_wstring(svgProps->minX.value_or(0)) + L" " +
                  std::to_wstring(svgProps->minY.value_or(0)) + L" " +
                  std::to_wstring(svgProps->vbWidth.value_or(0)) + L" " +
                  std::to_wstring(svgProps->vbHeight.value_or(0));
              nestedElem->SetAttributeValue(
                  SvgStrings::viewBoxAttributeName,
                  D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, vb.c_str());
            }
            if (svgProps->vbWidth) nestedElem->SetAttributeValue(
                SvgStrings::widthAttributeName,
                D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG,
                std::to_wstring(svgProps->vbWidth.value()).c_str());
            if (svgProps->vbHeight) nestedElem->SetAttributeValue(
                SvgStrings::heightAttributeName,
                D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG,
                std::to_wstring(svgProps->vbHeight.value()).c_str());
          }
          // D2D1 DrawSvgDocument 渲染嵌套 svg 的 shape 时自动用嵌套 viewBox，
          // 但 DWrite text 在文档外画，须手动累积嵌套 viewBox 变换到 text 的
          // accumulatedTransform，否则 text 坐标用外层 viewBox 映射会偏（d2 有框没文本）。
          // 嵌套 svg 无 preserveAspectRatio 信息时按 xMidYMid meet（D2D1 默认）算 scale。
          D2D1_MATRIX_3X2_F nestedTextTransform = accumulatedTransform;
          if (svgProps && (svgProps->vbWidth || svgProps->vbHeight)) {
            float nvbW = svgProps->vbWidth.value_or(0);
            float nvbH = svgProps->vbHeight.value_or(0);
            float nW = svgProps->vbWidth.value_or(0);  // 嵌套 svg width=其 vbWidth（d2 如此）
            float nH = svgProps->vbHeight.value_or(0);
            if (nvbW > 0 && nvbH > 0 && nW > 0 && nH > 0) {
              float ns = std::min(nW / nvbW, nH / nvbH);
              float ox = (nW - nvbW * ns) / 2 - svgProps->minX.value_or(0) * ns;
              float oy = (nH - nvbH * ns) / 2 - svgProps->minY.value_or(0) * ns;
              nestedTextTransform = D2D1::Matrix3x2F::Scale(ns, ns) *
                  D2D1::Matrix3x2F::Translation(ox, oy) * accumulatedTransform;
            }
          }
          RecurseRenderNode(root, child, document, *nestedElem, nestedTextTransform, inheritedTextCtx);
        }
      }
    }
  }
}

void SvgView::Draw(
    const winrt::Microsoft::ReactNative::Composition::ViewComponentView &view,
    ID2D1DeviceContext &context,
    winrt::Windows::Foundation::Size const &size) noexcept {

  com_ptr<ID2D1DeviceContext> deviceContext;
  deviceContext.copy_from(&context);

  auto deviceContext5 = deviceContext.as<ID2D1DeviceContext5>();

  winrt::com_ptr<ID2D1SvgDocument> spSvgDocument;
  deviceContext5->CreateSvgDocument(nullptr, D2D1_SIZE_F{size.Width, size.Height}, spSvgDocument.put());

  winrt::com_ptr<ID2D1SvgElement> spRoot;
  spSvgDocument->GetRoot(spRoot.put());

  if (m_props->vbWidth != std::nullopt || m_props->vbHeight != std::nullopt) {
    std::wstring viewBoxStr = std::to_wstring(m_props->minX.value_or(0)) + L" " +
        std::to_wstring(m_props->minY.value_or(0)) + L" " + std::to_wstring(m_props->vbWidth.value_or(0)) + L" " +
        std::to_wstring(m_props->vbHeight.value_or(0));
    spRoot->SetAttributeValue(
        SvgStrings::viewBoxAttributeName,
        D2D1_SVG_ATTRIBUTE_STRING_TYPE::D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG,
        viewBoxStr.c_str());
  }

  spRoot->SetAttributeValue(
      SvgStrings::widthAttributeName, D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, std::to_wstring(size.Width).c_str());
  spRoot->SetAttributeValue(
      SvgStrings::heightAttributeName, D2D1_SVG_ATTRIBUTE_STRING_TYPE_SVG, std::to_wstring(size.Height).c_str());

  if (m_props->color) {
    spRoot->SetAttributeValue(
        SvgStrings::colorAttributeName, D2DHelpers::AsD2DColor(m_props->color.AsWindowsColor(Theme())));
  }

  if (m_props->align != std::nullopt || m_props->meetOrSlice != std::nullopt) {
    D2D1_SVG_PRESERVE_ASPECT_RATIO preserveAspectRatio;
    preserveAspectRatio.defer = false;
    preserveAspectRatio.align = m_aspectAlign;

    preserveAspectRatio.meetOrSlice = m_props->meetOrSlice.value() == MeetOrSlice::Meet
        ? D2D1_SVG_ASPECT_SCALING::D2D1_SVG_ASPECT_SCALING_MEET
        : D2D1_SVG_ASPECT_SCALING::D2D1_SVG_ASPECT_SCALING_SLICE;
    spRoot->SetAttributeValue(SvgStrings::preserveAspectRatioAttributeName, preserveAspectRatio);
  }

  m_textRecords.clear();

  for (auto const &child : view.Children()) {
    auto renderable = child.UserData().as<RenderableView>();
    if (renderable->IsSupported()) {
      RecurseRenderNode(this, child, *spSvgDocument, *spRoot, D2D1::Matrix3x2F::Identity(), TextContext{});
    }
  }

  deviceContext5->DrawSvgDocument(spSvgDocument.get());

  // D2D1 SVG 不支持 text/tspan，DrawSvgDocument 忽略它们。RecurseRenderNode 已把文字
  // 信息（含累积 transform + 继承的 font/fill/anchor）收集到 m_textRecords，这里用
  // DWrite 叠加自绘。
  DrawTextRecords(deviceContext, size);
}

// viewBox→surface 映射（复现 D2D1 DrawSvgDocument 的 preserveAspectRatio）。
// 返回 transform：surfacePoint = transform.TransformPoint(svgPoint)。outScale 给 fontSize。
D2D1::Matrix3x2F SvgView::ComputeViewBoxTransform(winrt::Windows::Foundation::Size size, float &outScale) noexcept {
  float W = size.Width, H = size.Height;
  float vbW = m_props->vbWidth.value_or(W);
  float vbH = m_props->vbHeight.value_or(H);
  float minX = m_props->minX.value_or(0);
  float minY = m_props->minY.value_or(0);
  float sx = (vbW > 0) ? W / vbW : 1.0f;
  float sy = (vbH > 0) ? H / vbH : 1.0f;
  float scaleX, scaleY;
  if (m_aspectAlign == D2D1_SVG_ASPECT_ALIGN_NONE) {
    scaleX = sx;
    scaleY = sy;
  } else {
    bool slice = m_props->meetOrSlice && m_props->meetOrSlice.value() == MeetOrSlice::Slice;
    float s = slice ? std::max(sx, sy) : std::min(sx, sy);
    scaleX = scaleY = s;
  }
  outScale = scaleX;
  // align 偏移：xMin=0, xMid=(W-vbW*scale)/2, xMax=W-vbW*scale；y 同。
  auto a = m_aspectAlign;
  float offsetX = 0, offsetY = 0;
  bool xMid = (a == D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MIN || a == D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MID ||
              a == D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MAX);
  bool xMax = (a == D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MIN || a == D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MID ||
              a == D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MAX);
  if (xMid) offsetX = (W - vbW * scaleX) / 2;
  else if (xMax) offsetX = W - vbW * scaleX;
  bool yMid = (a == D2D1_SVG_ASPECT_ALIGN_X_MIN_Y_MID || a == D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MID ||
              a == D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MID);
  bool yMax = (a == D2D1_SVG_ASPECT_ALIGN_X_MIN_Y_MAX || a == D2D1_SVG_ASPECT_ALIGN_X_MID_Y_MAX ||
              a == D2D1_SVG_ASPECT_ALIGN_X_MAX_Y_MAX);
  if (yMid) offsetY = (H - vbH * scaleY) / 2;
  else if (yMax) offsetY = H - vbH * scaleY;
  return D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
      D2D1::Matrix3x2F::Translation(offsetX - minX * scaleX, offsetY - minY * scaleY);
}

// fontWeight（"bold"/"normal"/数字串，可能为空）折算 DWrite weight。
static DWRITE_FONT_WEIGHT ParseFontWeight(const std::wstring &weight) noexcept {
  if (weight == L"bold") return DWRITE_FONT_WEIGHT_BOLD;
  if (!weight.empty() && weight != L"normal") {
    try {
      int numeric = std::stoi(weight);
      if (numeric >= 600) return DWRITE_FONT_WEIGHT_BOLD;
    } catch (...) {
      // 非数字串按 normal 处理。
    }
  }
  return DWRITE_FONT_WEIGHT_NORMAL;
}

void SvgView::DrawTextRecords(
    const winrt::com_ptr<ID2D1DeviceContext> &deviceContext,
    winrt::Windows::Foundation::Size size) noexcept {
  if (m_textRecords.empty()) return;

  float scale;
  auto vbTransform = ComputeViewBoxTransform(size, scale);

  com_ptr<IDWriteFactory> dwriteFactory;
  DWriteCreateFactory(
      DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
      reinterpret_cast<::IUnknown **>(dwriteFactory.put_void()));
  com_ptr<ID2D1SolidColorBrush> brush;
  deviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), brush.put());

  for (auto const &rec : m_textRecords) {
    D2D1_POINT_2F pos = vbTransform.TransformPoint({rec.x, rec.y});
    float fs = rec.fontSize * scale;
    if (fs <= 0) fs = 16.0f;
    com_ptr<IDWriteTextFormat> textFormat;
    dwriteFactory->CreateTextFormat(
        rec.fontFamily.empty() ? L"Arial" : rec.fontFamily.c_str(),
        nullptr, ParseFontWeight(rec.fontWeight), DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        fs, L"", textFormat.put());
    brush->SetColor(rec.fill);

    // 用 TextLayout 先 measure 宽度 + 基线，再按 textAnchor + baseline 定位。
    com_ptr<IDWriteTextLayout> textLayout;
    dwriteFactory->CreateTextLayout(
        rec.content.c_str(), static_cast<UINT32>(rec.content.size()),
        textFormat.get(), size.Width, size.Height, textLayout.put());
    DWRITE_TEXT_METRICS tm{};
    textLayout->GetMetrics(&tm);
    DWRITE_LINE_METRICS lm{};
    UINT32 lineCount = 0;
    textLayout->GetLineMetrics(&lm, 1, &lineCount);
    float baseline = (lineCount > 0) ? lm.baseline : fs * 0.8f;
    float lineH = (lineCount > 0) ? lm.height : fs * 1.2f;

    // textAnchor: start(默认)=左对齐 pos.x；middle=pos.x-width/2；end=pos.x-width（右对齐）。
    float left = pos.x;
    if (rec.textAnchor == L"middle") left = pos.x - tm.width / 2;
    else if (rec.textAnchor == L"end") left = pos.x - tm.width;

    // 竖直定位：SVG y 默认是字母基线（alphabetic）。dominant/alignment-baseline 为
    // central/middle（zrender/vega 的刻度标签全靠它居中）时按行盒中心对齐 y；
    // text-before-edge/hanging 按行盒顶部；text-after-edge/bottom 按行盒底部。
    // DWrite DrawTextLayout 的 origin 是 layout box top-left，基线 = origin.y + baseline。
    float top = pos.y - baseline;
    const std::wstring &bm = rec.baselineMode;
    if (bm == L"central" || bm == L"middle") {
      top = pos.y - lineH / 2;
    } else if (bm == L"text-before-edge" || bm == L"hanging" || bm == L"text-top") {
      top = pos.y;
    } else if (bm == L"text-after-edge" || bm == L"bottom" || bm == L"text-bottom") {
      top = pos.y - lineH;
    }

    deviceContext->DrawTextLayout(
        D2D1::Point2F(left, top), textLayout.get(), brush.get(),
        D2D1_DRAW_TEXT_OPTIONS_NONE);
  }
}

winrt::Microsoft::ReactNative::Composition::Theme SvgView::Theme() const noexcept {
  if (auto view = m_wkView.get()) {
    return view.Theme();
  }
  return nullptr;
}

void SvgView::Invalidate() {
  if (auto view = m_wkView.get()) {
    auto size = winrt::Windows::Foundation::Size{ m_layoutMetrics.Frame.Width, m_layoutMetrics.Frame.Height };

    if (!m_isMounted) {
      return;
    }

    if (size.Height == 0 || size.Width == 0) {
      return;
    }

    auto drawingSurface = m_compContext.CreateDrawingSurfaceBrush(
        { m_layoutMetrics.Frame.Width * m_layoutMetrics.PointScaleFactor, m_layoutMetrics.Frame.Height * m_layoutMetrics.PointScaleFactor },
        winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
        winrt::Windows::Graphics::DirectX::DirectXAlphaMode::Premultiplied);

    POINT offset;
    {
      ::Microsoft::ReactNative::Composition::AutoDrawDrawingSurface autoDraw(drawingSurface, 1.0, &offset);
      if (auto deviceContext = autoDraw.GetRenderTarget()) {
        auto transform =
          winrt::Windows::Foundation::Numerics::make_float3x2_translation({static_cast<float>(offset.x / m_layoutMetrics.PointScaleFactor), static_cast<float>(offset.y / m_layoutMetrics.PointScaleFactor)});
        deviceContext->SetTransform(D2DHelpers::AsD2DTransform(transform));

        deviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black, 0.0f));

        com_ptr<ID2D1DeviceContext> spDeviceContext;
        spDeviceContext.copy_from(deviceContext);

        const auto dpi = m_layoutMetrics.PointScaleFactor * 96.0f;
        float oldDpiX, oldDpiY;
        deviceContext->GetDpi(&oldDpiX, &oldDpiY);
        deviceContext->SetDpi(dpi, dpi);

        Draw(view, *spDeviceContext, size);

        // restore dpi to old state
        deviceContext->SetDpi(oldDpiX, oldDpiY);

      }
    }

    m_visual.Brush(drawingSurface);
  }
}
} // namespace winrt::RNSVG::implementation
