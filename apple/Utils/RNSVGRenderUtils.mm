#import "RNSVGRenderUtils.h"

@implementation RNSVGRenderUtils

+ (CIContext *)sharedCIContext
{
  static CIContext *sharedCIContext = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    sharedCIContext = [[CIContext alloc] init];
  });

  return sharedCIContext;
}

+ (CGFloat)getScreenScale
{
  CGFloat scale = 0.0;
#if TARGET_OS_OSX
  scale = [[NSScreen mainScreen] backingScaleFactor];
#else
  if (@available(iOS 13.0, *)) {
    scale = [UITraitCollection currentTraitCollection].displayScale;
  } else {
#if !TARGET_OS_VISION
    scale = [[UIScreen mainScreen] scale];
#endif
  }
#endif // TARGET_OS_OSX
  return scale;
}

+ (CGFloat)scaleOf:(CGAffineTransform)transform
{
  CGFloat determinant = transform.a * transform.d - transform.b * transform.c;
  CGFloat scale = sqrt(fabs(determinant));
  return scale > 0 ? scale : 1.0;
}

+ (CGImage *)renderToImage:(RNSVGRenderable *)renderable
                       ctm:(CGAffineTransform)ctm
                      rect:(CGRect)rect
                    scale:(CGFloat)scale
                      clip:(CGRect *)clip
{
  // The bitmap is sized in device pixels for `rect`'s space (the caller picks
  // `scale`: the backing scale for a top-level node whose rect is in points,
  // the CTM's own scale for a nested node whose rect is in user space — d2
  // emits nested svgs, and its connection paths are masked). Concatenating the
  // CTM then lands the content on the bitmap exactly.
  RNSVGUIGraphicsBeginImageContextWithOptions(rect.size, NO, scale);
  CGContextRef cgContext = UIGraphicsGetCurrentContext();
  CGContextConcatCTM(cgContext, CGAffineTransformInvert(CGContextGetCTM(cgContext)));
  CGContextConcatCTM(cgContext, ctm);

  if (clip) {
    CGContextClipToRect(cgContext, *clip);
  }
  [renderable renderLayerTo:cgContext rect:rect];
  CGImageRef contentImage = CGBitmapContextCreateImage(cgContext);
  RNSVGUIGraphicsEndImageContext();
  return contentImage;
}

@end
