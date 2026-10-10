#import "RNSVGRenderable.h"

@interface RNSVGRenderUtils : NSObject

+ (CIContext *)sharedCIContext;
+ (CGFloat)getScreenScale;
/** Magnitude of the scale a transform applies; 1 when the transform is degenerate. */
+ (CGFloat)scaleOf:(CGAffineTransform)transform;
+ (CGImage *)renderToImage:(RNSVGRenderable *)renderable
                       ctm:(CGAffineTransform)ctm
                      rect:(CGRect)rect
                    scale:(CGFloat)scale
                      clip:(CGRect *)clip;

@end
