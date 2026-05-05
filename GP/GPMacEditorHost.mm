#include "GPMacEditorHost.hpp"

#if defined(OS_MAC) && IPLUG_EDITOR

#import <Cocoa/Cocoa.h>

#include "wdlstring.h"

static NSView* GPResolveParentView(void* parent)
{
  id candidate = (id) parent;

  if (candidate == nil)
    return nil;

  if ([candidate isKindOfClass:[NSView class]])
    return (NSView*) candidate;

  if ([candidate isKindOfClass:[NSWindow class]])
    return [(NSWindow*) candidate contentView];

  return nil;
}

static NSTextField* GPStaticLabel(NSString* text, CGFloat fontSize, NSColor* color, NSFontWeight weight)
{
  NSTextField* label = [[[NSTextField alloc] initWithFrame:NSZeroRect] autorelease];
  [label setEditable:NO];
  [label setBordered:NO];
  [label setDrawsBackground:NO];
  [label setSelectable:NO];
  [label setStringValue:text ?: @""];
  [label setTextColor:color];
  [label setFont:[NSFont systemFontOfSize:fontSize weight:weight]];
  return label;
}

static NSString* GPParameterDisplayString(GP& plugin, int paramIdx)
{
  WDL_String display;
  plugin.GetParam(paramIdx)->GetDisplayWithLabel(display);
  return [NSString stringWithUTF8String:display.Get() ?: ""];
}

@interface GPMacParameterSlider : NSSlider
{
@public
  GP* mPlugin;
  NSInteger mParamIdx;
}
@end

@implementation GPMacParameterSlider

- (void)mouseDown:(NSEvent*) event
{
  if (mPlugin)
    mPlugin->BeginInformHostOfParamChangeFromUI((int) mParamIdx);

  [super mouseDown:event];

  if (mPlugin)
    mPlugin->EndInformHostOfParamChangeFromUI((int) mParamIdx);
}

@end

@interface GPMacEditorController : NSObject
{
@public
  GP*                 mPlugin;
  NSView*              mRootView;
  NSTextField*         mTitleLabel;
  NSTextField*         mSubtitleLabel;
  NSTextField*         mKnobAName;
  NSTextField*         mKnobAValue;
  GPMacParameterSlider* mKnobASlider;
  NSTextField*         mKnobBName;
  NSTextField*         mKnobBValue;
  GPMacParameterSlider* mKnobBSlider;
  BOOL                 mSyncingFromPlugin;
}
- (instancetype) initWithPlugin:(GP&) plugin;
- (NSView*) openInParent:(NSView*) parent;
- (void) close;
- (void) resizeToWidth:(CGFloat) width height:(CGFloat) height;
- (void) syncFromPlugin:(int) paramIdx;
@end

@implementation GPMacEditorController

- (instancetype) initWithPlugin:(GP&) plugin
{
  self = [super init];
  if (self)
    mPlugin = &plugin;
  return self;
}

- (void) dealloc
{
  [self close];
  [super dealloc];
}

- (void) layoutContentWithWidth:(CGFloat) width height:(CGFloat) height
{
  const CGFloat safeWidth = MAX(width, 360.0);
  const CGFloat safeHeight = MAX(height, 180.0);
  const CGFloat marginX = 28.0;
  const CGFloat contentWidth = MAX(safeWidth - (marginX * 2.0), 200.0);
  const CGFloat rowWidth = contentWidth;
  const CGFloat labelWidth = rowWidth * 0.55;
  const CGFloat valueWidth = rowWidth * 0.25;
  const CGFloat sliderWidth = rowWidth;

  [mRootView setFrame:NSMakeRect(0.0, 0.0, safeWidth, safeHeight)];

  [mTitleLabel sizeToFit];
  [mSubtitleLabel sizeToFit];

  CGFloat y = safeHeight - 38.0;
  [mTitleLabel setFrameOrigin:NSMakePoint(marginX, y)];

  y -= 24.0;
  [mSubtitleLabel setFrameOrigin:NSMakePoint(marginX, y)];

  y -= 48.0;

  [mKnobAName setFrame:NSMakeRect(marginX, y, labelWidth, 20.0)];
  [mKnobAValue setFrame:NSMakeRect(marginX + rowWidth - valueWidth, y, valueWidth, 20.0)];
  y -= 28.0;
  [mKnobASlider setFrame:NSMakeRect(marginX, y, sliderWidth, 24.0)];

  y -= 48.0;

  [mKnobBName setFrame:NSMakeRect(marginX, y, labelWidth, 20.0)];
  [mKnobBValue setFrame:NSMakeRect(marginX + rowWidth - valueWidth, y, valueWidth, 20.0)];
  y -= 28.0;
  [mKnobBSlider setFrame:NSMakeRect(marginX, y, sliderWidth, 24.0)];
}

- (void) sliderChanged:(GPMacParameterSlider*) slider
{
  if (mSyncingFromPlugin || mPlugin == nullptr)
    return;

  const int paramIdx = (int) slider->mParamIdx;
  const double value = [slider doubleValue];
  mPlugin->SendParameterValueFromUI(paramIdx, mPlugin->GetParam(paramIdx)->ToNormalized(value));

  if (paramIdx == kKnobAMix)
    [mKnobAValue setStringValue:GPParameterDisplayString(*mPlugin, kKnobAMix)];
  else if (paramIdx == kKnobBMix)
    [mKnobBValue setStringValue:GPParameterDisplayString(*mPlugin, kKnobBMix)];
}

- (void) createViewIfNeeded
{
  if (mRootView)
    return;

  mRootView = [[NSView alloc] initWithFrame:NSMakeRect(0.0, 0.0, PLUG_WIDTH, PLUG_HEIGHT)];
  [mRootView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
  [mRootView setWantsLayer:YES];
  [[mRootView layer] setBackgroundColor:[[NSColor colorWithCalibratedRed:0.08 green:0.09 blue:0.11 alpha:1.0] CGColor]];

  mTitleLabel = [GPStaticLabel(@"GlintPlug", 24.0, [NSColor whiteColor], NSFontWeightSemibold) retain];
  mSubtitleLabel = [GPStaticLabel(@"macOS native editor while Glint's mac backend is pending", 12.0, [NSColor colorWithCalibratedWhite:0.72 alpha:1.0], NSFontWeightRegular) retain];
  mKnobAName = [GPStaticLabel(@"Knob A", 13.0, [NSColor whiteColor], NSFontWeightMedium) retain];
  mKnobAValue = [GPStaticLabel(@"", 13.0, [NSColor colorWithCalibratedRed:0.46 green:0.72 blue:1.0 alpha:1.0], NSFontWeightSemibold) retain];
  [mKnobAValue setAlignment:NSTextAlignmentRight];
  mKnobBName = [GPStaticLabel(@"Knob B", 13.0, [NSColor whiteColor], NSFontWeightMedium) retain];
  mKnobBValue = [GPStaticLabel(@"", 13.0, [NSColor colorWithCalibratedRed:0.52 green:0.93 blue:0.67 alpha:1.0], NSFontWeightSemibold) retain];
  [mKnobBValue setAlignment:NSTextAlignmentRight];

  mKnobASlider = [[GPMacParameterSlider alloc] initWithFrame:NSZeroRect];
  [mKnobASlider setMinValue:-100.0];
  [mKnobASlider setMaxValue:100.0];
  [mKnobASlider setContinuous:YES];
  [mKnobASlider setTarget:self];
  [mKnobASlider setAction:@selector(sliderChanged:)];
  mKnobASlider->mPlugin = mPlugin;
  mKnobASlider->mParamIdx = kKnobAMix;

  mKnobBSlider = [[GPMacParameterSlider alloc] initWithFrame:NSZeroRect];
  [mKnobBSlider setMinValue:-100.0];
  [mKnobBSlider setMaxValue:100.0];
  [mKnobBSlider setContinuous:YES];
  [mKnobBSlider setTarget:self];
  [mKnobBSlider setAction:@selector(sliderChanged:)];
  mKnobBSlider->mPlugin = mPlugin;
  mKnobBSlider->mParamIdx = kKnobBMix;

  [mRootView addSubview:mTitleLabel];
  [mRootView addSubview:mSubtitleLabel];
  [mRootView addSubview:mKnobAName];
  [mRootView addSubview:mKnobAValue];
  [mRootView addSubview:mKnobASlider];
  [mRootView addSubview:mKnobBName];
  [mRootView addSubview:mKnobBValue];
  [mRootView addSubview:mKnobBSlider];
}

- (void) syncAll
{
  [self syncFromPlugin:kKnobAMix];
  [self syncFromPlugin:kKnobBMix];
}

- (NSView*) openInParent:(NSView*) parent
{
  [self createViewIfNeeded];

  const CGFloat width = parent ? NSWidth([parent bounds]) : PLUG_WIDTH;
  const CGFloat height = parent ? NSHeight([parent bounds]) : PLUG_HEIGHT;
  [self resizeToWidth:width height:height];

  if (parent && [mRootView superview] != parent)
  {
    [mRootView removeFromSuperview];
    [parent addSubview:mRootView];
  }

  [self syncAll];
  return mRootView;
}

- (void) close
{
  if (!mRootView)
    return;

  [mRootView removeFromSuperview];

  [mKnobASlider release];
  [mKnobBSlider release];
  [mTitleLabel release];
  [mSubtitleLabel release];
  [mKnobAName release];
  [mKnobAValue release];
  [mKnobBName release];
  [mKnobBValue release];
  [mRootView release];

  mKnobASlider = nil;
  mKnobBSlider = nil;
  mTitleLabel = nil;
  mSubtitleLabel = nil;
  mKnobAName = nil;
  mKnobAValue = nil;
  mKnobBName = nil;
  mKnobBValue = nil;
  mRootView = nil;
}

- (void) resizeToWidth:(CGFloat) width height:(CGFloat) height
{
  if (!mRootView)
    return;

  [self layoutContentWithWidth:width height:height];
}

- (void) syncFromPlugin:(int) paramIdx
{
  if (!mRootView || mPlugin == nullptr)
    return;

  mSyncingFromPlugin = YES;

  if (paramIdx == kKnobAMix)
  {
    [mKnobASlider setDoubleValue:mPlugin->GetParam(kKnobAMix)->Value()];
    [mKnobAValue setStringValue:GPParameterDisplayString(*mPlugin, kKnobAMix)];
  }
  else if (paramIdx == kKnobBMix)
  {
    [mKnobBSlider setDoubleValue:mPlugin->GetParam(kKnobBMix)->Value()];
    [mKnobBValue setStringValue:GPParameterDisplayString(*mPlugin, kKnobBMix)];
  }

  mSyncingFromPlugin = NO;
}

@end

GPMacEditorHost::GPMacEditorHost(GP& plugin)
  : mPlugin(plugin)
{
}

GPMacEditorHost::~GPMacEditorHost()
{
  Close();
}

void* GPMacEditorHost::Open(void* parent)
{
  if (!mController)
    mController = [[GPMacEditorController alloc] initWithPlugin:mPlugin];

  GPMacEditorController* controller = (GPMacEditorController*) mController;
  return [controller openInParent:GPResolveParentView(parent)];
}

void GPMacEditorHost::Close()
{
  if (!mController)
    return;

  GPMacEditorController* controller = (GPMacEditorController*) mController;
  [controller close];
  [controller release];
  mController = nullptr;
}

void GPMacEditorHost::OnParentResized(int width, int height)
{
  if (!mController)
    return;

  GPMacEditorController* controller = (GPMacEditorController*) mController;
  [controller resizeToWidth:width height:height];
}

void GPMacEditorHost::SyncFromPlugin(int paramIdx)
{
  if (!mController)
    return;

  GPMacEditorController* controller = (GPMacEditorController*) mController;
  [controller syncFromPlugin:paramIdx];
}

#endif